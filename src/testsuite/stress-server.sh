#!/bin/sh
#
# ngIRCd Test Suite
# Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

# parse command line
[ "$1" -gt 0 ] 2>/dev/null && CLIENTS="$1" || CLIENTS=5
[ "$2" -gt 0 ] 2>/dev/null && MAX="$2" || MAX=-1

# detect source directory
[ -z "$srcdir" ] && srcdir=`dirname "$0"`
set -u

# get our name
name=`basename "$0"`

# create directories
[ -d logs ] || mkdir logs
[ -d tests ] || mkdir tests

# test for required external tools
type expect >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "${name}: \"expect\" not found."
	exit 77
fi
type telnet >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "${name}: \"telnet\" not found.";
	exit 77
fi

# hello world! :-)
echo "stressing server with $CLIENTS clients (be patient!):"

# read in functions
. "${srcdir}/functions.inc"

# create scripts for expect(1)
no=0
while [ ${no} -lt $CLIENTS ]; do
	cat "${srcdir}/stress-A.e" >tests/${no}.e
	echo "send \"nick test${no}\\r\"" >>tests/${no}.e
	cat "${srcdir}/stress-B.e" >>tests/${no}.e
	no=`expr ${no} + 1`
done

# run first script and check if it succeeds
echo_n "checking stress script ..."
expect tests/0.e >logs/stress-0.log 2>/dev/null
if [ $? -ne 0 ]; then
	echo " failure!"
	exit 1
else
	echo " ok."
fi

no=0
while [ ${no} -lt $CLIENTS ]; do
	expect tests/${no}.e >logs/stress-${no}.log 2>/dev/null &

	no=`expr ${no} + 1`
	echo "started client $no/$CLIENTS."

	[ $MAX -gt 0 ] && "$srcdir/wait-tests.sh" $MAX
done

echo_n "waiting for clients to complete: ."
touch logs/check-idle.log
while true; do
	expect "${srcdir}/check-idle.e" >>logs/check-idle.log; res=$?
	echo "====================" >>logs/check-idle.log
	[ $res -ne 99 ] && break

	# there are still clients connected. Wait ...
	sleep 3
	echo_n "."
done

[ $res -eq 0 ] && echo " ok." || echo " failure!"
exit $res
