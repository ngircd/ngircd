#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.2 2002/09/12 02:27:47 alex Exp $

echo "      stopping server ..."

ps a > procs.tmp
pid=`cat procs.tmp | grep ngircd-TEST | awk "{ print \\\$1 }"`
kill $pid > /dev/null 2>&1

# -eof-
