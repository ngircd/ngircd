# ngIRCd: cleartext on a TLS connection after another client's TLS reset

A local, deterministic before/after test for the bug fixed in
ngircd/ngircd#351. It uses Docker only and an unmodified ngIRCd
(no debug hooks or LD_PRELOAD), against LibreSSL (Alpine) and OpenSSL (Debian).

## Run it

```
git fetch origin pull/351/head:pr-351
./run.sh origin/master pr-351                 # 5 runs each, both TLS libraries
./run.sh -r 10 -l openssl origin/master pr-351
```

`run.sh` uses `git archive` to export both refs from the ngIRCd repository this
directory is in (or `-C`/`$NGIRCD_REPO`) into `work/src/`. It then builds one image per
TLS library, with both trees built `--with-openssl`, and runs `repro.py` in a fresh
container for each test. At the end it prints a table. Full client and server logs are
in `work/logs/`.

## What the test does (repro.py)

| Client | Role |
|---|---|
| **B** (victim) | TLS client on a raw socket with `ssl.MemoryBIO`, so every raw byte from the server is recorded. Registers, then **stops reading**. |
| **C** (feeder) | Plain-text oper with `+F`, so it isn't throttled. Sends 400-byte PRIVMSGs to B, with a PING round-trip after each, until the server's Send-Q to B has not grown for 12 messages (read from `/proc/net/tcp`). At that point `send()` to B returns EAGAIN, and a few KB of B's output wait in ngIRCd's write buffer, well below the 32 KB sendq limit. |
| **A** (trigger) | Completes a TLS handshake, then closes with `SO_LINGER 0`, which sends an RST. |

The bug chain on unpatched ngIRCd:

1. The server's `SSL_read()` on A fails with ECONNRESET. `ConnSSL_Free()` →
   `SSL_shutdown()` writes to the dead socket and gets EPIPE. That error stays in the
   OpenSSL error queue.
2. C sends one more PRIVMSG to B. `Conn_Write()` → `Handle_Write()` → `SSL_write()` on B
   returns -1 (really WANT_WRITE). Because of the stale queue entry, `SSL_get_error()`
   returns SYSCALL. ngIRCd logs `SSL error: …Broken pipe [in ConnSSL_Write()]` and calls
   `ConnSSL_Free()` on B.
3. `errno` is still EAGAIN, so B is not closed. C sends `CANARY-n` lines. B then starts
   reading again, and the server sends B's pending output with plain `write()`.

B is not reading and C uses the plain-text port, so the only TLS operations between A's
reset and B's write are A's own. That makes the sequence deterministic.

Verdicts:

- **LEAK:** the canary text was found in B's raw TCP stream, which means cleartext inside
  the TLS connection.
- **KILLED:** the server closed B.
- **OK:** B received every canary through TLS and still answers a PING.

The `ctl` row is a control on the unpatched build: A sends QUIT and closes cleanly instead
of resetting. It must be OK, which shows the leak is caused by the reset and not by the
slow reader or the test itself.

## Environment notes

- `--sysctl net.ipv4.tcp_wmem="4096 16384 16384"` (container network namespace only) caps
  the server's socket send buffer. Without it, Linux auto-tuning keeps growing the send
  buffer to 4 MB on loopback, so the socket to B never fills before ngIRCd's 32 KB sendq
  limit would kill B. A busy client on a real WAN link reaches the same "send() returns
  EAGAIN with data still queued" state through normal congestion.
- B's `SO_RCVBUF` is 4096, so the receive side fills quickly too.

## Files

- `run.sh`: build both refs, run the matrix, print the table.
- `repro.py`: the test client. It runs inside the container. `-v` prints the fill progress,
  and `--no-reset` runs the control.
- `docker/`: `Dockerfile.libressl`, `Dockerfile.openssl`, `build.sh`. A self-signed
  certificate is generated at image build time.
- `ngircd.conf`: plain port 6667, TLS port 6697, and one operator.
