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
# $Id: autogen.sh,v 1.11 2004/03/19 11:47:51 alex Exp $
#

#
# Usage: [VAR=<value>] ./autogen.sh [<configure-args>]
#
# This script generates the ./configure script using GNU automake and
# GNU autoconf. It tries to be smart in finding the correct/usable/available
# installed versions of these tools on your system.
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
# - ALICA=<cmd>, AUTOHEADER=<cmd>, AUTOMAKE=<cmd>, AUTOCONF=<cmd>
#   Name and optionally path to the particular tool.
# - PREFIX=<path>
#   Search the GNU autoconf and GNU automake tools in <path> first. If the
#   generated ./configure script will be called, pass "--prefix=<path>" to it.
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

Search()
{
	[ $# -eq 2 ] || exit 1

	searchlist="$1"
	major="$2"
	minor=99

	which /bin/ls >/dev/null 2>&1
	[ $? -eq 0 ] && exists="which" || exists="type"

	[ -n "$PREFIX" ] && searchlist="${PREFIX}/$1 ${PREFIX}/bin/$1 $searchlist"

	for name in $searchlist; do
		$exists "${name}" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			echo "${name}"
			return 0
		fi
	done

	while [ $minor -ge 0 ]; do
		for name in $searchlist; do
			$exists "${name}${major}${minor}" >/dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo "${name}${major}${minor}"
				return 0
			fi
			$exists "${name}-${major}.${minor}" >/dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo "${name}-${major}.${minor}" >/dev/null 2>&1
				return 0
			fi
		done
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
[ "$VERBOSE" = "1" ] && echo "ACLOCAL=$ACLOCAL"
[ -z "$AUTOHEADER" ] && AUTOHEADER=`Search autoheader 2`
[ "$VERBOSE" = "1" ] && echo "AUTOHEADER=$AUTOHEADER"
[ -z "$AUTOMAKE" ] && AUTOMAKE=`Search automake 1`
[ "$VERBOSE" = "1" ] && echo "AUTOMAKE=$AUTOMAKE"
[ -z "$AUTOCONF" ] && AUTOCONF=`Search autoconf 2`
[ "$VERBOSE" = "1" ] && echo "AUTOCONF=$AUTOCONF"

# Call ./configure when parameters have been passed to this script and
# GO isn't already defined.
[ -z "$GO" -a $# -gt 0 ] && GO=1

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

if [ $? -eq 0 -a -x ./configure ]; then
	# Success: if we got some parameters we call ./configure and pass
	# all of them to it.
	if [ "$GO" = "1" ]; then
		[ -n "$PREFIX" ] && p=" --prefix=$PREFIX" || p=""
		[ -n "$*" ] && a=" $*" || a=""
		c="./configure${p}${a}"
		echo "Calling \"$c\" ..."
		$c
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
