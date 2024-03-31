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

# detect source directory
[ -z "$srcdir" ] && srcdir=`dirname "$0"`
set -u

name=`basename "$0"`
test=`echo ${name} | cut -d '.' -f 1`
[ -d logs ] || mkdir logs

if [ ! -r "$test" ]; then
	echo "$test: test not found" >>tests-skipped.lst
	echo "${name}: test \"$test\" not found!";  exit 77
	exit 1
fi

# read in functions
. "${srcdir}/functions.inc"

type expect >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "$test: \"expect\" not found" >>tests-skipped.lst
	echo "${name}: \"expect\" not found."
	exit 77
fi
type telnet >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "$test: \"telnet\" not found" >>tests-skipped.lst
	echo "${name}: \"telnet\" not found."
	exit 77
fi

case "$test" in
	*ssl*)
		type openssl >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "$test: \"openssl\" not found" >>tests-skipped.lst
			echo "${name}: \"openssl\" not found."
			exit 77
		fi
		;;
esac

# prepare expect script
e_in="${srcdir}/${test}.e"
e_tmp="${test}.e_"
e_exec="$e_in"
if test -t 1 2>/dev/null; then
	sed -e 's|^expect |puts -nonewline stderr "."; expect |g' \
		"$e_in" >"$e_tmp"
	[ $? -eq 0 ] && e_exec="$e_tmp"
fi

echo_n "running ${test} ..."
expect "$e_exec" >logs/${test}.log; r=$?
[ $r -eq 0 ] && echo " ok." || echo " failure!"

rm -f "$e_tmp"
exit $r
