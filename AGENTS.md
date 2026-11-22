# AGENTS.md

This file provides guidance to AI coding agents when working with code in
this repository.

## Project

ngIRCd is a portable, lightweight IRC server written in C (GPLv2+) for a wide
range of Unix-like platforms, including old ones: stick to C89 (e.g. no
declarations after statements) and use the `src/portab` compatibility layer.

Threat model: only clients are untrusted. Peer servers, services, and the
local configuration are trusted, and the server-to-server protocol is
insecure by design: handle input from server links robustly, but it is no
attack surface. Optional features like iconv (`CHARCONV`) and PAM are off by
default but enabled by distribution packages (see `contrib/Debian/rules`),
which matters when rating the impact of a bug.

Reference documentation: `doc/Commands.txt` (all commands), `doc/Modes.txt`,
`doc/Protocol.txt` (IRC+ server protocol extensions), `doc/Capabilities.txt`.

## Building

`configure.ac` and most `Makefile.am` files are **generated** by
`./autogen.sh` (GNU autoconf/automake):

- Edit `configure.ng`, and `Makefile.ng` in `src/ngircd`, `src/portab`,
  `src/ipaddr`, `src/tool`, and `src/testsuite`; add new source files to
  `*_SOURCES`/`noinst_HEADERS` there, and new test files to `EXTRA_DIST`.
- `man/*.tmpl` and `doc/sample-ngircd.conf.tmpl` are the sources of the man
  pages and the sample configuration file.

```sh
./autogen.sh [configure-args]  # generate configure, run it if args are given
make                           # build src/ngircd/ngircd
make check                     # build and run all tests (all subdirs)
make distcheck                 # validate distribution archive + tests (CI)
src/ngircd/ngircd -n -f <conf> # run in the foreground (-t: check config)
```

Optional features: `--enable-ipv6 --with-iconv --with-ident --with-openssl`
(or `--with-gnutls`) `--with-pam --with-tcp-wrappers --with-zlib`; for
development `--enable-debug`, `--enable-sniffer` (`-s` logs all IRC traffic),
and `--enable-strict-rfc`. CI (`.github/workflows/ci.yml`) builds with gcc and
clang on Ubuntu and macOS.

## Tests

`src/testsuite` starts real servers (binary copies `T-ngircd1..3` with
`ngircd-test<N>.conf`, fixed ports 6789/6790) and drives them with `expect(1)`
scripts (`<name>.e`, run by `tests.sh`, logs in `logs/`). Missing tools skip
tests, SSL tests need SSL support. Run single tests with their servers:

```sh
cd src/testsuite
make check TESTS="start-server1 start-server2 channel-test stop-server2 stop-server1"
```

The full suite is slow: run single tests while working, and the full suite
once at the end. Never run two suites at once, and use other ports for
manually started servers. A new test needs its `.e` script, a symlink rule,
and `TESTS`/`EXTRA_DIST` entries in `src/testsuite/Makefile.ng`.

## Architecture

Single-process, single-threaded, event-driven daemon in `src/ngircd`, linking
the helper libraries `src/portab` (portability), `src/tool` (strings), and
`src/ipaddr` (IPv4/IPv6 addresses).

- **Main loop:** `main()` (`ngircd.c`) reads the config and runs
  `Conn_Handler()` (`conn.c`), which loops on `io_dispatch()` (`io.c`, event
  backends epoll, kqueue, /dev/poll, poll, select).
- **Connections vs. clients:** `conn.c` manages sockets and buffers by
  `CONN_ID` (`My_Connections[]`); `client.c` manages `CLIENT` structures of
  local and remote users, servers, and services (types in `client.h`).
  Remote clients have no connection (`Client_Conn()` is `NONE`) and were
  already processed by their own server: apply local policies (cloaking, DNS
  checks, default modes) only to local clients, or servers end up with
  different views. Optional layers: `conn-ssl.c` (OpenSSL/GnuTLS),
  `conn-zip.c` (zlib server links), `conn-encoding.c` (iconv).
- **Commands:** `parse.c` splits lines into a `REQUEST` and dispatches via the
  `My_Commands[]` table (handler, allowed client types, argument limits,
  penalty) to `IRC_<COMMAND>(CLIENT *, REQUEST *)` in `irc-*.c`, which return
  `CONNECTED`/`DISCONNECTED`; `irc-macros.h` validates arguments. Numerics
  from peer servers are handled in `numeric.c`. Parameters can be empty
  (e.g. a trailing `:`).
- **Lifetime:** after anything that can destroy the current client
  (`Conn_Close()`, `IRC_KillClient()`, ...), return `DISCONNECTED` at once
  and don't touch the `CLIENT` again. A channel may vanish when its last
  member leaves or mode "P" is removed, so look it up again.
- **Output:** `IRC_Write*` functions (`irc-write.c`); all reply formats are
  in `messages.h`.
- **Child processes:** DNS/IDENT lookups (`resolve.c`) and PAM (`login.c`,
  `pam.c`) run in children forked by `Proc_Fork()` (`proc.c`), whose results
  come back through pipes in the event loop.
- **Configuration:** `conf.c` parses `ngircd.conf` into `Conf_*` globals;
  `SIGHUP` triggers `Conf_Rehash()` (`sighandlers.c`), which reads everything
  into the same globals again: free the old data of each section (see
  `opers_free()`), and keep values documented as generated on startup (e.g.
  the random `CloakHostSalt`).
- **Data structures:** `channel.c` (channels, memberships), `lists.c` (mask
  lists for bans/invites/exceptions and G-/K-lines in `class.c`), `array.c`,
  `hash.c`.

## Coding Conventions

- Tabs (width 8, see `.editorconfig`), K&R style (`contrib/ngindent.sh`).
- Prototypes use `PARAMS((...))`, exported functions are marked `GLOBAL`
  (`portab.h`), and prefixed with their module (`Conn_*`, `Client_*`,
  `Channel_*`, `Conf_*`, `IRC_*`, ...). Functions carry Doxygen comments.
- Keep the lists of modes in sync: `CHANMODES`/`USERMODES` and the modes
  allowed in "DefaultChannelModes" (`CHANMODES_DEFAULT`) in `defines.h` and
  the documentation of that option, the ISUPPORT `CHANMODES=` groups
  (`messages.h`), and `doc/Modes.txt`.
- Document new options in `doc/sample-ngircd.conf.tmpl` and
  `man/ngircd.conf.5.tmpl`, and new commands in `doc/Commands.txt`.
- Add a `ChangeLog` entry for changes worth mentioning (fixes, features,
  changed behavior) in the same commit, at the top of the upcoming release;
  `NEWS` lists only notable user-facing features and changes, not plain
  bug fixes. Credit reporters and reference issues on lines of their own at
  the end, e.g. "Thanks <name> for reporting this!" and "Closes #<N>.".
  `AUTHORS.md` is checked at release time (`doc/HowToRelease.txt`).

## File Formatting

Follow the project's existing conventions first: formatter and linter
configuration (e.g. `.editorconfig`, `pyproject.toml`, `rustfmt.toml`), then
the style of the surrounding code. Where nothing is defined, keep lines within
80 columns.

Wrap prose in text files at 80 columns. Don't wrap code, tables, URLs, or other
lines that would break if split.

Markdown:

- Leave a blank line below every heading.

## Version Control

If a `.jj` directory is present, the clone is a colocated Jujutsu/Git repo:
use `jj` rather than mutating `git` commands then.

### Commits

- Subject: imperative mood, capitalized, no trailing period, ≤ 50 characters
  (72 at most).
- Blank line between subject and body; wrap the body at 72 columns.
- Body explains what changed and why, not how.
- One logical change per commit.
- Reference GitHub issues in a paragraph of its own, before the trailers:
  "Closes #<N>." only if the change fully resolves the issue, otherwise
  "See #<N>.".

End the description of commits made with AI assistance with a trailer naming
the model that was actually used, separated from the body by a blank line:

```text
Co-authored-by: <model name> <vendor no-reply address>
```

Contributions must be reviewed and tested by a human before they are
submitted, see "Using AI tools and LLMs" in `CONTRIBUTING.md`.
