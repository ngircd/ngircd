#!/bin/sh
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2004 Alexander Barton <alex@barton.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# $Id: autogen.sh,v 1.9 2004/03/15 18:59:12 alex Exp $
#

Search()
{
	[ $# -eq 2 ] || exit 1

	name="$1"
	major="$2"
	minor=99

	type "${name}" >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "${name}"
		return 0
	fi

	while [ $minor -ge 0 ]; do
		type "${name}${major}${minor}" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			echo "${name}${major}${minor}"
			return 0
		fi
		type "${name}-${major}.${minor}" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			echo "${name}-${major}.${minor}" >/dev/null 2>&1
			return 0
		fi
		minor=`expr $minor - 1`
	done
	return 1
}

Notfound()
{
	echo "Error: $* not found!"
	echo "Please install recent versions of GNU autoconf and GNU automake."
	exit 1
}

# Reset locale settings to suppress warning messages of Perl
unset LC_ALL
unset LANG

# We want to use GNU automake 1.7, if available (WANT_AUTOMAKE is used by
# the wrapper scripts of Gentoo Linux):
WANT_AUTOMAKE=1.7
export WANT_AUTOMAKE

# Try to detect the needed tools when no environment variable already
# spezifies one:
echo "Searching tools ..."
[ -z "$ACLOCAL" ] && ACLOCAL=`Search aclocal 1`
[ -z "$AUTOHEADER" ] && AUTOHEADER=`Search autoheader 2`
[ -z "$AUTOMAKE" ] && AUTOMAKE=`Search automake 1`
[ -z "$AUTOCONF" ] && AUTOCONF=`Search autoconf 2`

# Some debugging output ...
if [ -n "$DEBUG" ]; then
	echo "ACLOCAL=$ACLOCAL"
	echo "AUTOHEADER=$AUTOHEADER"
	echo "AUTOMAKE=$AUTOMAKE"
	echo "AUTOCONF=$AUTOCONF"
fi

# Verify that all tools have been found
[ -z "$AUTOCONF" ] && Notfounf autoconf
[ -z "$AUTOHEADER" ] && Notfound autoheader
[ -z "$AUTOMAKE" ] && Notfound automake
[ -z "$AUTOCONF" ] && Notfound autoconf

export AUTOCONF AUTOHEADER AUTOMAKE AUTOCONF

# Generate files
echo "Generating files ..."
$ACLOCAL && \
	$AUTOHEADER && \
	$AUTOMAKE --add-missing && \
	$AUTOCONF

if [ $? -eq 0 ]; then
	# Success: if we got some parameters we call ./configure and pass
	# all of them to it.
	if [ -n "$*" -a -x ./configure ]; then
		echo "Calling generated \"configure\" script ..."
		./configure $*
		exit $?
	else
		echo "Okay, autogen.sh done; now run the \"configure\" script."
		exit 0
	fi
else
	# Failure!?
	echo "Error! Check your installation of GNU automake and autoconf!"
	exit 1
fi

# -eof-
