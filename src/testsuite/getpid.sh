#!/bin/sh
# ngIRCd Test Suite
# $Id: getpid.sh,v 1.3 2003/04/22 19:27:50 alex Exp $

# wurde ein Name uebergeben?
[ $# -ne 1 ] && exit 1

# Flags fuer "ps" ermitteln
if [ `uname` = "FreeBSD" ]; then
  PS_FLAGS="-a"; PS_PIDCOL="1"; HEAD_FLAGS="-n 1"
elif [ `uname` = "A/UX" ]; then
  PS_FLAGS="-ae"; PS_PIDCOL="1"; HEAD_FLAGS="-1"
else
  PS_FLAGS="-f"; PS_PIDCOL="2"; HEAD_FLAGS="-n 1"
  ps $PS_FLAGS > /dev/null 2>&1
  if [ $? -ne 0 ]; then PS_FLAGS="a"; PS_PIDCOL="1"; fi
fi

# PID ermitteln
ps $PS_FLAGS > procs.tmp
cat procs.tmp | grep "$1" | awk "{print \$$PS_PIDCOL}" | sort -n > pids.tmp
pid=`head $HEAD_FLAGS pids.tmp`
rm -rf procs.tmp pids.tmp

# ermittelte PID validieren
[ "$pid" -gt 1 ] > /dev/null 2>&1
[ $? -ne 0 ] && exit 1

echo $pid
exit 0

# -eof-
