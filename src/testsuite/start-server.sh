#!/bin/sh
# ngIRCd Test Suite
# $Id: start-server.sh,v 1.1 2002/09/09 10:16:24 alex Exp $

echo "      starting server ..."

echo "This is an ngIRCd Test Server" > ngircd-test.motd

../ngircd/ngircd -np -f ngircd-test.conf > ngircd-test.log 2>&1 &
sleep 1

pid=`ps a | grep ngircd-test | head -n 1 | cut -d ' ' -f 1`
kill -0 $pid > /dev/null 2>&1

# -eof-
