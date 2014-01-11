#!/bin/sh
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors
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
CLEAN=1

PLATFORM=
COMPILER="unknown"
VERSION="unknown"
DATE=`date "+%y-%m-%d"`
COMMENT=

R_CONFIGURE=
R_MAKE=
R_CHECK=
R_RUN=

[ -n "$MAKE" ] || MAKE="make"
export MAKE CC

while [ $# -gt 0 ]; do
	case "$1" in
		"-v")
			VERBOSE=1
			;;
		"-x")
			CLEAN=
			;;
		*)
			echo "Usage: $NAME [-v] [-x]"
			echo
			echo "  -v   Verbose output"
			echo "  -x   Don't regenerate build system, even when possible"
			echo
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

echo "$NAME: Checking for GIT tree ..."
if [ -d .git ]; then
	echo "$NAME: Checking for \"git\" command ..."
	git version >/dev/null 2>&1
	if [ $? -eq 0 -a -n "$CLEAN" ]; then
		echo "$NAME: Running \"git clean\" ..."
		[ -n "$VERBOSE" ] && git clean -dxf || git clean -dxf >/dev/null
	fi
fi

echo "$NAME: Checking for \"./configure\" script ..."
if [ ! -r ./configure ]; then
	echo "$NAME: Running \"./autogen.sh\" ..."
	[ -n "$VERBOSE" ] && ./autogen.sh || ./autogen.sh >/dev/null
fi

if [ -r ./configure ]; then
	echo "$NAME: Running \"./configure\" script ..."
	[ -n "$VERBOSE" ] && ./configure || ./configure >/dev/null
	if [ $? -eq 0 -a -r ./Makefile ]; then
		R_CONFIGURE=1
		echo "$NAME: Running \"$MAKE\" ..."
		[ -n "$VERBOSE" ] && "$MAKE" || "$MAKE" >/dev/null
		if [ $? -eq 0 -a -x src/ngircd/ngircd ]; then
			R_MAKE=1
			echo "$NAME: Running \"$MAKE check\" ..."
			[ -n "$VERBOSE" ] && "$MAKE" check || "$MAKE" check >/dev/null
			if [ $? -eq 0 ]; then
				R_CHECK=1
				R_RUN=$R_CHECK
			else
				./src/ngircd/ngircd --help 2>/dev/null \
				 | grep "^ngIRCd" >/dev/null
				[ $? -eq 0 ] && R_RUN=1
			fi
		fi
	fi
fi

# Get target platform information
if [ -r "src/config.h" ]; then
	CPU=`grep "HOST_CPU" "src/config.h" | cut -d'"' -f2`
	OS=`grep "HOST_OS" "src/config.h" | cut -d'"' -f2`
	VENDOR=`grep "HOST_VENDOR" "src/config.h" | cut -d'"' -f2`
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
		# GCC, or compiler that mimics GCC
		$CC --version 2>&1 | grep -i "Open64" >/dev/null
		if [ $? -eq 0 ]; then
			COMPILER="Open64"
		else
			COMPILER=$($CC --version | head -1 \
			  | cut -d')' -f2 | cut -d' ' -f2)
			COMPILER="gcc $COMPILER"
		fi
	else
		# Non-GCC compiler
		$CC --version 2>&1 | grep -i "clang" >/dev/null
		if [ $? -eq 0 ]; then
			COMPILER=$($CC --version 2>/dev/null | head -1 \
			  | cut -d'(' -f1 | cut -d'-' -f1 \
			  | sed -e 's/version //g' | sed -e 's/Apple /A-/g' \
			  | sed -e 's/Debian //g' | sed -e 's/LLVM /clang /g')
		fi
		$CC -version 2>&1 | grep -i "tcc" >/dev/null
		if [ $? -eq 0 ]; then
			COMPILER=$($CC -version 2>/dev/null | head -1 \
			  | cut -d'(' -f1 | sed -e 's/version //g')
		fi
		if [ "$COMPILER" = "unknown" ]; then
			v="`$CC --version 2>/dev/null | head -1`"
			[ -z "$v" ] && v="`$CC -version 2>/dev/null | head -1`"
			[ -n "$v" ] && COMPILER="$v"
		fi
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
	COMMENT="1"
else
	grep "^#define HAVE_SYS_DEVPOLL_H 1" src/config.h >/dev/null 2>&1
	[ $? -eq 0 ] && COMMENT="4"
	grep "^#define HAVE_EPOLL_CREATE 1" src/config.h >/dev/null 2>&1
	[ $? -eq 0 ] && COMMENT="5"
	grep "^#define HAVE_KQUEUE 1" src/config.h >/dev/null 2>&1
	[ $? -eq 0 ] && COMMENT="3"
fi

[ -n "$R_CONFIGURE" ] && C="Y" || C="N"
[ -n "$R_MAKE" ] && M="Y" || M="N"
[ -n "$R_CHECK" ] && T="Y" || T="N"
[ -n "$R_RUN" ] && R="Y" || R="N"
[ -n "$COMMENT" ] && COMMENT=" $COMMENT"

echo
echo "                                the executable works (\"runs\") as expected --+"
echo "                                  tests run successfully (\"make check\") --+ |"
echo "                                             ngIRCd compiles (\"make\") --+ | |"
echo "                                                  ./configure works --+ | | |"
echo "                                                                      | | | |"
echo "Platform                    Compiler     ngIRCd     Date     Tester   C M T R *"
echo "--------------------------- ------------ ---------- -------- -------- - - - - -"
type printf >/dev/null 2>&1
if [ $? -eq 0 ]; then
	printf "%-27s %-12s %-10s %s %-8s %s %s %s %s%s" \
	 "$PLATFORM" "$COMPILER" "$VERSION" "$DATE" "$USER" \
	 "$C" "$M" "$T" "$R" "$COMMENT"
else
	echo "$PLATFORM $COMPILER $VERSION $DATE $USER" \
	 "$C" "$M" "$T" "$R" "$COMMENT"
fi
echo; echo
