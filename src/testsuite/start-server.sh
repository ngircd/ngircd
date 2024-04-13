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

echo_n "starting server ${id} ..."

# remove old logfiles, if this is the first server (ID 1)
[ "$id" = "1" ] && rm -rf logs *.log

# check weather getpid.sh returns valid PIDs. If not, don't start up the
# test-server, because we won't be able to kill it at the end of the test.
./getpid.sh sh >/dev/null
if [ $? -ne 0 ]; then
	echo " getpid.sh failed!"
	exit 1
fi

# check if there is a test-server already running
./getpid.sh T-ngircd${id} >/dev/null 2>&1
if [ $? -eq 0 ]; then
	echo " failure: test-server ${id} already running!"
	exit 1
fi

# generate MOTD for test-server
echo "This is an ngIRCd Test Server" >ngircd-test${id}.motd

# glibc memory checking, see malloc(3)
MALLOC_CHECK_=3
export MALLOC_CHECK_

# starting up test-server ...
./T-ngircd${id} -n -f "${srcdir}/ngircd-test${id}.conf" "$@" \
 >ngircd-test${id}.log 2>&1 &
sleep 1

# validate running test-server
r=1
pid=`./getpid.sh T-ngircd${id}`
[ -n "$pid" ] && kill -0 $pid >/dev/null 2>&1; r=$?
[ $r -eq 0 ] && echo " ok." || echo " failure!"
exit $r
