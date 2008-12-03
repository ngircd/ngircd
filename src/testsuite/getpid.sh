#!/bin/sh
# ngIRCd Test Suite
# $Id: getpid.sh,v 1.5 2006/08/05 00:15:28 alex Exp $

# did we get a name?
[ $# -ne 1 ] && exit 1

# detect flags for "ps" and "head"
UNAME=`uname`
if [ $UNAME = "FreeBSD" ]; then
  PS_FLAGS="-a"; PS_PIDCOL="1"; HEAD_FLAGS="-n 1"
elif [ $UNAME = "A/UX" ]; then
  PS_FLAGS="-ae"; PS_PIDCOL="1"; HEAD_FLAGS="-1"
elif [ $UNAME = "GNU" ]; then
  PS_FLAGS="-ax"; PS_PIDCOL="2"; HEAD_FLAGS="-n 1"
elif [ $UNAME = "SunOS" ]; then
  PS_FLAGS="-af"; PS_PIDCOL=2; HEAD_FLAGS="-n 1"
else
  PS_FLAGS="-f"; PS_PIDCOL="2"; HEAD_FLAGS="-n 1"
  ps $PS_FLAGS > /dev/null 2>&1
  if [ $? -ne 0 ]; then PS_FLAGS="a"; PS_PIDCOL="1"; fi
fi

# debug output
#echo "$0: UNAME=$UNAME"
#echo "$0: PS_FLAGS=$PS_FLAGS"
#echo "$0: PS_PIDCOL=$PS_PIDCOL"
#echo "$0: HEAD_FLAGS=$HEAD_FLAGS"

# search PID
ps $PS_FLAGS > procs.tmp
cat procs.tmp | \
  grep -v "$0" | grep "$1" | \
  awk "{print \$$PS_PIDCOL}" | \
  sort -n > pids.tmp
pid=`head $HEAD_FLAGS pids.tmp`
rm -rf procs.tmp pids.tmp

# validate PID
[ "$pid" -gt 1 ] > /dev/null 2>&1
[ $? -ne 0 ] && exit 1

echo $pid
exit 0

# -eof-
