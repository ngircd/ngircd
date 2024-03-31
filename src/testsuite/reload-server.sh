#!/bin/sh
# ngIRCd Test Suite

[ -z "$srcdir" ] && srcdir=`dirname "$0"`
set -u

# read in functions
. "${srcdir}/functions.inc"

if [ -n "$1" ]; then
	id="$1"; shift
else
	id="1"
fi

echo_n "reloading server ${id} ..."

# reload (sighup) test-server ...
pid=`./getpid.sh T-ngircd${id}`
if [ -z "$pid" ]; then
	echo " failure: no running server found!?"
	exit 1
fi
kill -HUP $pid >/dev/null 2>&1; r=$?
if [ $r -eq 0 ]; then
	sleep 2
	echo " ok".
	kill -0 $pid && exit 0
fi
echo " failure: server ${id} could not be reloaded!"
exit 1
