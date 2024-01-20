#!/bin/sh
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2016 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

# This script analyzes the build process of ngIRCd and generates output
# suitable for inclusion in doc/Platforms.txt -- please send reports
# to the ngIRCd mailing list: <ngircd@lists.barton.de>.

NAME=$(basename "$0")
VERBOSE=
CLEAN=1

PLATFORM=
COMPILER="unknown"
VERSION="unknown"
DATE=$(date "+%y-%m-%d")
COMMENT=

R_CONFIGURE=
R_MAKE=
R_CHECK=
R_CHECK_Y="?"
R_RUN=

SRC_D=$(dirname "$0")
MY_D="$PWD"

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

for cmd in telnet expect; do
	command -v "$cmd" >/dev/null 2>&1 \
		|| echo "$NAME: WARNING: $cmd(1) not found, \"make check\" won't run all tests!"
done

echo "$NAME: Checking ngIRCd base source directory ..."
grep "ngIRCd" "$SRC_D/ChangeLog" >/dev/null 2>&1
if [ $? -ne 0 ]; then
	grep "ngIRCd" "$SRC_D/../ChangeLog" >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "$NAME: ngIRCd base source directory not found!?"
		exit 1
	fi
	SRC_D="$SRC_D/.."
fi
echo "$NAME:  - source directory: $SRC_D"
echo "$NAME:  - working directory: $MY_D"

echo "$NAME: Checking for GIT tree ..."
if [ -d "$SRC_D/.git" ]; then
	echo "$NAME: Checking for \"git\" command ..."
	git version >/dev/null 2>&1
	if [ $? -eq 0 ] && [ -n "$CLEAN" ]; then
		echo "$NAME: Running \"git clean\" ..."
		cd "$SRC_D" || exit 1
		if [ -n "$VERBOSE" ]; then
			git clean -dxf
		else
			git clean -dxf >/dev/null
		fi
		cd "$MY_D" || exit 1
	fi
fi

echo "$NAME: Checking for \"$SRC_D/configure\" script ..."
if [ ! -r "$SRC_D/configure" ]; then
	echo "$NAME: Running \"$SRC_D/autogen.sh\" ..."
	cd "$SRC_D" || exit 1
	if [ -n "$VERBOSE" ]; then
		./autogen.sh
	else
		./autogen.sh >/dev/null
	fi
	if [ $? -ne 0 ]; then
		echo "$NAME: \"$SRC_D/autogen.sh\" script failed, aborting!"
		exit 1
	fi
	cd "$MY_D" || exit 1
fi

if [ -r "$SRC_D/configure" ]; then
	echo "$NAME: Running \"$SRC_D/configure\" script ..."
	if [ -n "$VERBOSE" ]; then
		"$SRC_D/configure" -C
	else
		"$SRC_D/configure" -C >/dev/null
	fi
	if [ $? -eq 0 ] && [ -r ./Makefile ]; then
		R_CONFIGURE=1
		rm -f "src/ngircd/ngircd"
		echo "$NAME: Running \"$MAKE\" ..."
		if [ -n "$VERBOSE" ]; then
			"$MAKE"
		else
			"$MAKE" >/dev/null
		fi
		if [ $? -eq 0 ] && [ -x src/ngircd/ngircd ]; then
			R_MAKE=1
			echo "$NAME: Running \"$MAKE check\" ..."
			if [ -n "$VERBOSE" ]; then
				"$MAKE" check
			else
				"$MAKE" check >/dev/null
			fi
			if [ $? -eq 0 ]; then
				R_CHECK=1
				R_RUN=$R_CHECK
				[ -r ./src/testsuite/tests-skipped.lst ] \
					&& R_CHECK_Y="y" || R_CHECK_Y="Y"
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
	CPU=$(grep "HOST_CPU" "src/config.h" | cut -d'"' -f2)
	OS=$(grep "HOST_OS" "src/config.h" | cut -d'"' -f2)
	VENDOR=$(grep "HOST_VENDOR" "src/config.h" | cut -d'"' -f2)
	PLATFORM="$CPU/$VENDOR/$OS"
fi
if [ -z "$PLATFORM" ]; then
	PLATFORM="$(uname 2>/dev/null) $(uname -r 2>/dev/null), $(uname -m 2>/dev/null)"
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
			  | sed -e 's/version //g; s/^\([A-Z]\)[A-Za-z]* clang/\1-clang/g; s/LLVM /clang /g')
		fi
		$CC -version 2>&1 | grep -i "tcc" >/dev/null
		if [ $? -eq 0 ]; then
			COMPILER=$($CC -version 2>/dev/null | head -1 \
			  | cut -d'(' -f1 | sed -e 's/version //g')
		fi
		if [ "$COMPILER" = "unknown" ]; then
			v="$($CC --version 2>/dev/null | head -1)"
			[ -z "$v" ] && v="$($CC -version 2>/dev/null | head -1)"
			[ -n "$v" ] && COMPILER="$v"
		fi
	fi
fi

# Get ngIRCd version information
eval "$(grep "^VERSION = " Makefile | sed -e 's/ //g')"
case "$VERSION" in
	*~*-*)
		VERSION=$(echo "$VERSION" | cut -b1-10)
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
[ -n "$R_CHECK" ] && T="$R_CHECK_Y" || T="N"
if [ -n "$R_RUN" ]; then
	# Mark "runs" with "Y" only when the test suite succeeded:
	[ "$T" = "N" ] && R="?" || R="Y"
else
	R="N"
fi
[ -n "$COMMENT" ] && COMMENT=" $COMMENT"

echo
echo "                                the executable works (\"runs\") as expected --+"
echo "                                  tests run successfully (\"make check\") --+ |"
echo "                                             ngIRCd compiles (\"make\") --+ | |"
echo "                                                  ./configure works --+ | | |"
echo "                                                                      | | | |"
echo "Platform                    Compiler     ngIRCd     Date     Tester   C M T R *"
echo "--------------------------- ------------ ---------- -------- -------- - - - - -"
command -v printf >/dev/null 2>&1
if [ $? -eq 0 ]; then
	printf "%-27s %-12s %-10s %s %-8s %s %s %s %s%s\n" \
	 "$PLATFORM" "$COMPILER" "$VERSION" "$DATE" "$LOGNAME" \
	 "$C" "$M" "$T" "$R" "$COMMENT"
else
	echo "$PLATFORM $COMPILER $VERSION $DATE $LOGNAME" \
	 "$C" "$M" "$T" "$R" "$COMMENT"
fi
echo

double_check() {
	echo "Please double check that the ngIRCd daemon starts up, runs and handles IRC"
	echo "connections successfully!"
}

if [ "$R_CHECK_Y" = "y" ]; then
	echo "WARNING: Some tests have been skipped!"
	double_check
	echo
fi
if [ "$R" = "?" ]; then
	echo "WARNING: The resulting binary passed simple tests, but the test suite failed!"
	double_check
	echo
fi
