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

[ "$1" -gt 0 ] 2>/dev/null && MAX="$1" || MAX=5

# detect source directory
[ -z "$srcdir" ] && srcdir=`dirname "$0"`
set -u

PS_FLAGS="-f"
ps $PS_FLAGS >/dev/null 2>&1
[ $? -ne 0 ] && PS_FLAGS="a"

# read in functions
. "${srcdir}/functions.inc"

msg=0
while true; do
	count=`ps $PS_FLAGS | grep "expect " | wc -l`
	count=`expr $count - 1`

	[ $count -le $MAX ] && break

	if [ $msg -lt 1 ]; then
		echo_n "      waiting for processes to settle: "
		msg=1
	fi

	# there are still clients connected. Wait ...
	echo_n "$count>$MAX "
	sleep 1
done

[ $msg -gt 0 ] && echo "done: $count"
exit 0
