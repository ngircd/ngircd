#!/bin/sh
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# Usage:
#   [VAR=<value>] ./autogen.sh [<configure-args>]
#
# This script generates the ./configure script using GNU automake and
# GNU autoconf. It tries to be smart in finding the correct/usable/available
# installed versions of these tools on your system.
#
# In addition, it enables or disables the "de-ANSI-fication" support of GNU
# automake, which is supported up to autoconf 1.11.x an has been removed
# in automake 1.12 -- make sure to use a version of automake supporting it
# when generating distribution archives!
#
# The following strategy is used for each of aclocal, autoheader, automake,
# and autoconf: first, "tool" (the regular name of the tool, e. g. "autoconf"
# or "automake") is checked. If this fails, "tool<major><minor>" (for example
# "automake16") and "tool-<major>.<minor>" (e. g. "autoconf-2.54") are tried
# with <major> being 2 for tool of GNU autoconf and 1 for tools of automake;
# <minor> is tried from 99 to 0. The first occurrence will be used.
#
# When you pass <configure-args> to autogen.sh it will call the generated
# ./configure script on success and pass these parameters to it.
#
# You can tweak the behaviour using these environment variables:
#
# - ACLOCAL=<cmd>, AUTOHEADER=<cmd>, AUTOMAKE=<cmd>, AUTOCONF=<cmd>
#   Name and optionally path to the particular tool.
# - PREFIX=<path>
#   Search the GNU autoconf and GNU automake tools in <path> first. If the
#   generated ./configure script will be called, pass "--prefix=<path>" to it.
# - EXIST=<tool>
#   Use <tool> to test for aclocal, autoheader etc. pp. ...
#   When not specified, either "type" or "which" is used.
# - VERBOSE=1
#   Output the detected names of the GNU automake and GNU autoconf tools.
# - GO=1
#   Call ./configure even if no arguments have been passed to autogen.sh.
#
# Examples:
#
# - ./autogen.sh
#   Generates the ./configure script.
# - GO=1 ./autogen.sh
#   Generates the ./configure script and runs it as "./configure".
# - VERBOSE=1 ./autogen.sh --with-ident
#   Show tool names, generates the ./configure script, and runs it with
#   these arguments: "./configure --with-ident".
# - ACLOCAL=aclocal-1.6 GO=1 PREFIX=$HOME ./autogen.sh
#   Uses "aclocal-1.6" as aclocal tool, generates the ./configure script,
#   and runs it with these arguments: "./configure --prefix=$HOME".
#

Check_Tool()
{
	searchlist="$1"
	major="$2"
	minor="$3"

	for name in $searchlist; do
		$EXIST "${name}${major}${minor}" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			echo "${name}${major}${minor}"
			return 0
		fi
		$EXIST "${name}-${major}.${minor}" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			echo "${name}-${major}.${minor}"
			return 0
		fi
	done
	return 1
}

Search()
{
	[ $# -lt 2 ] && return 1
	[ $# -gt 3 ] && return 1

	searchlist="$1"
	major="$2"
	minor_pref="$3"
	minor=99

	[ -n "$PREFIX" ] && searchlist="${PREFIX}/$1 ${PREFIX}/bin/$1 $searchlist"

	if [ -n "$minor_pref" ]; then
		Check_Tool "$searchlist" "$major" "$minor_pref" && return 0
	fi

	for name in $searchlist; do
		$EXIST "${name}" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			"${name}" --version 2>&1 \
			 | grep -v "environment variable" >/dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo "${name}"
				return 0
			fi
		fi
	done

	while [ $minor -ge 0 ]; do
		Check_Tool "$searchlist" "$major" "$minor" && return 0
		minor=$(expr $minor - 1)
	done
	return 1
}

Notfound()
{
	echo "Error: $* not found!"
	echo 'Please install supported versions of GNU autoconf, GNU automake'
	echo 'and pkg-config: see the INSTALL file for details.'
	exit 1
}

Run()
{
	[ "$VERBOSE" = "1" ] && echo " - running \"$*\" ..."
	"$@"
}

# Reset locale settings to suppress warning messages of Perl
unset LC_ALL
unset LANG

# Which command should be used to detect the automake/autoconf tools?
[ -z "$EXIST" ] && existlist="type which" || existlist="$EXIST"
EXIST=""
for t in $existlist; do
	$t /bin/ls >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		rm -f /tmp/test.$$
		$t /tmp/test.$$ >/dev/null 2>&1
		[ $? -ne 0 ] && EXIST="$t"
	fi
	[ -n "$EXIST" ] && break
done
if [ -z "$EXIST" ]; then
	echo "Didn't detect a working command to test for the autoconf/automake tools!"
	echo "Searchlist: $existlist"
	exit 1
fi
[ "$VERBOSE" = "1" ] && echo "Using \"$EXIST\" to test for tools."

# Try to detect the needed tools when no environment variable already
# specifies one:
echo "Searching for required tools ..."
[ -z "$ACLOCAL" ] && ACLOCAL=$(Search aclocal 1 11)
[ "$VERBOSE" = "1" ] && echo " - ACLOCAL=$ACLOCAL"
[ -z "$AUTOHEADER" ] && AUTOHEADER=$(Search autoheader 2)
[ "$VERBOSE" = "1" ] && echo " - AUTOHEADER=$AUTOHEADER"
[ -z "$AUTOMAKE" ] && AUTOMAKE=$(Search automake 1 11)
[ "$VERBOSE" = "1" ] && echo " - AUTOMAKE=$AUTOMAKE"
[ -z "$AUTOCONF" ] && AUTOCONF=$(Search autoconf 2)
[ "$VERBOSE" = "1" ] && echo " - AUTOCONF=$AUTOCONF"

AUTOCONF_VERSION=$(echo "$AUTOCONF" | cut -d'-' -f2-)
[ -n "$AUTOCONF_VERSION" ] && [ "$AUTOCONF_VERSION" != "autoconf" ] \
	&& export AUTOCONF_VERSION || unset AUTOCONF_VERSION
[ "$VERBOSE" = "1" ] && echo " - AUTOCONF_VERSION=$AUTOCONF_VERSION"
AUTOMAKE_VERSION=$(echo $AUTOMAKE | cut -d'-' -f2-)
[ -n "$AUTOMAKE_VERSION" ] && [ "$AUTOMAKE_VERSION" != "automake" ] \
	&& export AUTOMAKE_VERSION || unset AUTOMAKE_VERSION
[ "$VERBOSE" = "1" ] && echo " - AUTOMAKE_VERSION=$AUTOMAKE_VERSION"

[ $# -gt 0 ] && CONFIGURE_ARGS=" $*" || CONFIGURE_ARGS=""
[ -z "$GO" ] && [ -n "$CONFIGURE_ARGS" ] && GO=1

# Verify that all tools have been found
command -v pkg-config >/dev/null || Notfound pkg-config
[ -z "$ACLOCAL" ] && Notfound aclocal
[ -z "$AUTOHEADER" ] && Notfound autoheader
[ -z "$AUTOMAKE" ] && Notfound automake
[ -z "$AUTOCONF" ] && Notfound autoconf

AM_VERSION=$($AUTOMAKE --version | head -n 1 | sed -e 's/.* //g')
ifs=$IFS; IFS="."; set $AM_VERSION; IFS=$ifs
AM_MAJOR="$1"; AM_MINOR="$2"
echo "Detected automake $AM_VERSION ..."

AM_MAKEFILES="src/ipaddr/Makefile.ng src/ngircd/Makefile.ng src/testsuite/Makefile.ng src/tool/Makefile.ng"

# De-ANSI-fication?
if [ "$AM_MAJOR" -eq "1" ] && [ "$AM_MINOR" -lt "12" ]; then
	# automake < 1.12 => automatic de-ANSI-fication support available
	echo " - Enabling de-ANSI-fication support."
	sed -e "s|^__ng_PROTOTYPES__|AM_C_PROTOTYPES|g" configure.ng >configure.ac
	DEANSI_START=""
	DEANSI_END=""
else
	# automake >= 1.12 => no de-ANSI-fication support available
	echo " - Disabling de-ANSI-fication support."
	sed -e "s|^__ng_PROTOTYPES__|AC_C_PROTOTYPES|g" configure.ng >configure.ac
	DEANSI_START="#"
	DEANSI_END=" (disabled by ./autogen.sh script)"
fi
# Serial test harness?
if [ "$AM_MAJOR" -eq "1" ] && [ "$AM_MINOR" -ge "13" ]; then
	# automake >= 1.13 => enforce "serial test harness"
	echo " - Enforcing serial test harness."
	SERIAL_TESTS="serial-tests"
else
	# automake < 1.13 => no new test harness, nothing to do
	# shellcheck disable=SC2034
	SERIAL_TEST=""
fi

sed -e "s|^__ng_Makefile_am_template__|AUTOMAKE_OPTIONS = ${SERIAL_TESTS} ${DEANSI_START}ansi2knr${DEANSI_END}|g" \
	src/portab/Makefile.ng >src/portab/Makefile.am
for makefile_ng in $AM_MAKEFILES; do
	makefile_am=$(echo "$makefile_ng" | sed -e "s|\.ng\$|\.am|g")
	sed -e "s|^__ng_Makefile_am_template__|AUTOMAKE_OPTIONS = ${SERIAL_TESTS} ${DEANSI_START}../portab/ansi2knr${DEANSI_END}|g" \
		$makefile_ng >$makefile_am
done

export ACLOCAL AUTOHEADER AUTOMAKE AUTOCONF

# Generate files
echo "Generating files using \"$AUTOCONF\" and \"$AUTOMAKE\" ..."
Run $ACLOCAL && \
	Run $AUTOCONF && \
	Run $AUTOHEADER && \
	Run $AUTOMAKE --add-missing --no-force

if [ $? -eq 0 ] && [ -x ./configure ]; then
	# Success: if we got some parameters we call ./configure and pass
	# all of them to it.
	NAME=$(grep PACKAGE_STRING= configure | cut -d"'" -f2)
	if [ "$GO" = "1" ]; then
		[ -n "$PREFIX" ] && p=" --prefix=$PREFIX" || p=""
		c="./configure${p}${CONFIGURE_ARGS}"
		echo "Okay, autogen.sh for $NAME done."
		echo "Calling \"$c\" ..."
		$c
		exit $?
	else
		echo "Okay, autogen.sh for $NAME done."
		echo "Now run the \"./configure\" script."
		exit 0
	fi
else
	# Failure!?
	echo "Error! Check your installation of GNU automake and autoconf!"
	exit 1
fi

# -eof-
