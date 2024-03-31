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
[ "$1" -gt 0 ] 2>/dev/null && LOOPS="$1" || LOOPS=5
[ "$2" -gt 0 ] 2>/dev/null && WAIT="$2" || WAIT=5

# detect source directory
[ -z "$srcdir" ] && srcdir=`dirname "$0"`
set -u

loop=0
while [ ${loop} -lt $LOOPS ]; do
	loop=`expr ${loop} + 1`
	echo "      loop $loop/$LOOPS starting:"
	for s in "$srcdir"/*-test; do
		sh "$s"; r=$?
		[ $r -ne 0 ] && exit $r
		sleep 1
	done
	if [ ${loop} -lt $LOOPS ]; then
		echo "      waiting $WAIT seconds ..."
		sleep $WAIT
	fi
done
