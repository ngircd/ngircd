#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.4 2002/09/16 09:53:16 alex Exp $

echo "      stopping server ..."

PS_FLAGS=a; PS_PIDCOL=1
ps a > /dev/null 2>&1
if [ $? -ne 0 ]; then PS_FLAGS=-f; PS_PIDCOL=2; fi

ps $PS_FLAGS > procs.tmp
pid=`cat procs.tmp | grep ngircd-TEST | awk "{ print \\\$$PS_PIDCOL }"`
kill $pid > /dev/null 2>&1

# -eof-
