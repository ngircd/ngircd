#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.1 2002/09/09 10:16:24 alex Exp $

echo "      stopping server ..."

pid=`ps a | grep ngircd-test | head -n 1 | cut -d ' ' -f 1`
kill $pid > /dev/null 2>&1

# -eof-
