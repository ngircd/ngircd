#!/bin/sh
# ngIRCd Test Suite
# $Id: getpid.sh,v 1.1 2002/09/20 14:46:55 alex Exp $

# wurde ein Name uebergeben?
[ $# -ne 1 ] && exit 1

# Flags fuer "ps" ermitteln
if [ `uname` = "FreeBSD" ]; then
  PS_FLAGS=-a; PS_PIDCOL=1
else
  PS_FLAGS=-f; PS_PIDCOL=2
  ps $PS_FLAGS > /dev/null 2>&1
  if [ $? -ne 0 ]; then PS_FLAGS=a; PS_PIDCOL=1; fi
fi

# PID ermitteln
ps $PS_FLAGS > procs.tmp
pid=`cat procs.tmp | grep "$1" | awk "{ print \\\$$PS_PIDCOL }" | sort -n | head -n 1`

# ermittelte PID validieren
[ "$pid" -gt 1 ] > /dev/null 2>&1
[ $? -ne 0 ] && exit 1

echo $pid
exit 0

# -eof-
