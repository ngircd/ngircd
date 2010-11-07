#!/bin/sh
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2010 Alexander Barton <alex@barton.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

# This script analyzes the build process of ngIRCd and generates output
# suitable for inclusion in doc/Platforms.txt -- please send reports
# to the ngIRCd mailing list: <ngircd-ml@ngircd.barton.de>.

NAME=`basename "$0"`
VERBOSE=

PLATFORM=
COMPILER="unknown"
VERSION="unknown"
DATE=`date "+%y-%m-%d"`

CONFIGURE=
MAKE=
CHECK=
RUN=
COMMENT=

while [ $# -gt 0 ]; do
	case "$1" in
		"-v")
			VERBOSE=1
			;;
		*)
			echo "Usage: $NAME [-v]"
			exit 2
	esac
	shift
done

echo "$NAME: Checking ngIRCd base source directory ..."
grep "ngIRCd" ./ChangeLog >/dev/null 2>&1
if [ $? -ne 0 ]; then
	grep "ngIRCd" ../ChangeLog >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "$NAME: ngIRCd base source directory not found!?"
		exit 1
	fi
	cd ..
fi

echo "$NAME: Checking for \"./autogen.sh\" script ..."
if [ -r ./autogen.sh ]; then
	echo "$NAME: Running \"./autogen.sh\" ..."
	[ -n "$VERBOSE" ] && ./autogen.sh || ./autogen.sh >/dev/null
fi

if [ -r ./configure ]; then
	echo "$NAME: Running \"./configure\" script ..."
	[ -n "$VERBOSE" ] && ./configure || ./configure >/dev/null
	if [ $? -eq 0 -a -r ./Makefile ]; then
		CONFIGURE=1
		echo "$NAME: Running \"make\" ..."
		[ -n "$VERBOSE" ] && make || make >/dev/null
		if [ $? -eq 0 -a -x src/ngircd/ngircd ]; then
			MAKE=1
			echo "$NAME: Running \"make check\" ..."
			[ -n "$VERBOSE" ] && make check || make check >/dev/null
			if [ $? -eq 0 ]; then
				CHECK=1
				RUN=$CHECK
			else
				./src/ngircd/ngircd --help 2>/dev/null \
				 | grep "^ngircd" >/dev/null
				[ $? -eq 0 ] && RUN=1
			fi
		fi
	fi
fi

# Get target platform information
if [ -r "src/config.h" ]; then
	CPU=`grep "TARGET_CPU" "src/config.h" | cut -d'"' -f2`
	OS=`grep "TARGET_OS" "src/config.h" | cut -d'"' -f2`
	VENDOR=`grep "TARGET_VENDOR" "src/config.h" | cut -d'"' -f2`
	PLATFORM="$CPU/$VENDOR/$OS"
fi
if [ -z "$PLATFORM" ]; then
	PLATFORM="`uname 2>/dev/null` `uname -r 2>/dev/null`, `uname -m 2>/dev/null`"
fi

# Get compiler information
if [ -r "Makefile" ]; then
	CC=$(grep "^CC = " Makefile | cut -d' ' -f3)
	$CC --version 2>&1 | grep -i "GCC" >/dev/null
	if [ $? -eq 0 ]; then
		COMPILER=$($CC --version | head -1 | awk "{ print \$3 }" \
		 | cut -d'-' -f1)
		COMPILER="gcc $COMPILER"
	else
		case "$CC" in
		  gcc*)
			v="`$CC --version 2>/dev/null | head -1`"
			[ -n "$v" ] && COMPILER="gcc $v"
			;;
		esac
	fi
fi

# Get ngIRCd version information
eval $(grep "^VERSION = " Makefile | sed -e 's/ //g')
case "$VERSION" in
	*-*-*)
		VERSION=`echo "$VERSION" | cut -d'-' -f3 | cut -b2-`
		;;
esac
[ -n "$VERSION" ] || VERSION="unknown"

# Get IO interface information
if [ "$OS" = "linux-gnu" ]; then
	COMMENT="(1)"
else
	grep "^#define HAVE_SYS_DEVPOLL_H 1" src/config.h >/dev/null 2>&1
	[ $? -eq 0 ] && COMMENT="(4)"
	grep "^#define HAVE_EPOLL_CREATE 1" src/config.h >/dev/null 2>&1
	[ $? -eq 0 ] && COMMENT="(5)"
	grep "^#define HAVE_KQUEUE 1" src/config.h >/dev/null 2>&1
	[ $? -eq 0 ] && COMMENT="(3)"
fi

[ -n "$CONFIGURE" ] && C="Y" || C="N"
[ -n "$MAKE" ] && M="Y" || M="N"
[ -n "$CHECK" ] && T="Y" || T="N"
[ -n "$RUN" ] && R="Y" || R="N"
[ -n "$COMMENT" ] && COMMENT=" $COMMENT"

echo
echo "                              the executable works (\"runs\") as expected --+"
echo "                                tests run successfully (\"make check\") --+ |"
echo "                                           ngIRCd compiles (\"make\") --+ | |"
echo "                                                ./configure works --+ | | |"
echo "                                                                    | | | |"
echo "Platform                    Compiler     ngIRCd     Date     Tester C M T R See"
echo "--------------------------- ------------ ---------- -------- ------ - - - - ---"
type printf >/dev/null 2>&1
if [ $? -eq 0 ]; then
	printf "%-27s %-12s %-10s %s %-6s %s %s %s %s%s" \
	 "$PLATFORM" "$COMPILER" "$VERSION" "$DATE" "$USER" \
	 "$C" "$M" "$T" "$R" "$COMMENT"
else
	echo "$PLATFORM $COMPILER $VERSION $DATE $USER" \
	 "$C" "$M" "$T" "$R" "$COMMENT"
fi
echo; echo
