#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.4.2.1 2002/09/20 13:46:22 alex Exp $

echo "      stopping server ..."

PS_FLAGS=-f; PS_PIDCOL=2
ps $PS_FLAGS > /dev/null 2>&1
if [ $? -ne 0 ]; then PS_FLAGS=a; PS_PIDCOL=1; fi

ps $PS_FLAGS > procs.tmp
pid=`cat procs.tmp | grep ngircd-TEST | awk "{ print \\\$$PS_PIDCOL }"`
kill $pid > /dev/null 2>&1

# -eof-
