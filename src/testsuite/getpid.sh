#!/bin/sh
# ngIRCd Test Suite
#
# Try to detect the PID of a running process of the current user.
#

set -u

# did we get a name?
if [ $# -ne 1 ]; then
	echo "Usage: $0 <name>" >&2
	exit 1
fi

UNAME=`uname`

# Use pgrep(1) whenever possible
if [ -x /usr/bin/pgrep ]; then
	case "$UNAME" in
		"FreeBSD")
			PGREP_FLAGS="-a"
			;;
		*)
			PGREP_FLAGS=""
	esac
	if [ -n "${LOGNAME:-}" ] || [ -n "${USER:-}" ]; then
		# Try to narrow the search down to the current user ...
		exec /usr/bin/pgrep $PGREP_FLAGS -n -u "${LOGNAME:-$USER}" "$1"
	else
		# ... but neither LOGNAME nor USER were set!
		exec /usr/bin/pgrep $PGREP_FLAGS -n "$1"
	fi
fi

# pidof(1) could be a good alternative on elder Linux systems
if [ -x /bin/pidof ]; then
	exec /bin/pidof -s "$1"
fi

# fall back to ps(1) and parse its output:
# detect flags for "ps" and "head"
PS_PIDCOL=1
case "$UNAME" in
	"A/UX"|"GNU"|"SunOS")
		PS_FLAGS="-a"; PS_PIDCOL=2
		;;
	"Haiku")
		PS_FLAGS="-o Id -o Team"
		;;
	*)
		# Linux (GNU coreutils), Free/Net/OpenBSD, ...
		PS_FLAGS="-o pid,comm"
esac

# search PID
ps $PS_FLAGS >procs.tmp
grep -v "$$" procs.tmp | grep "$1" | \
	awk "{print \$$PS_PIDCOL}" | \
	sort -nr >pids.tmp
pid=`head -1 pids.tmp`
rm -rf procs.tmp pids.tmp

# validate PID
[ "$pid" -gt 1 ] >/dev/null 2>&1 || exit 1

echo $pid
exit 0
