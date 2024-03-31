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

echo_n "stopping server ${id} ..."

# stop test-server ...
pid=`./getpid.sh T-ngircd${id}`
if [ -z "$pid" ]; then
	echo " failure: no running server found!?"
	exit 1
fi
kill $pid >/dev/null 2>&1 || exit 1

# waiting ...
for i in 1 2 3 4 5; do
	kill -0 $pid >/dev/null 2>&1; r=$?
	if [ $r -ne 0 ]; then
		echo " ok".
		exit 0
	fi
	sleep 1
done
echo " failure: server ${id} still running!?"
exit 1
