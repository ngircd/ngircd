#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.3 2002/09/13 06:04:49 alex Exp $

echo "      stopping server ..."

ps ax > procs.tmp
pid=`cat procs.tmp | grep ngircd-TEST | awk "{ print \\\$1 }"`
kill $pid > /dev/null 2>&1

# -eof-
