#!/bin/sh
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2019 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# This script uses GNU indent(1) to format C source code files of ngIRCd.
# Usage:
#  - ./contrib/ngindent.sh [<file> [<file> [...]]]
#  - cat ./src/ngircd/<c_file> | ./contrib/ngindent.sh

# Use a coding-style based on "Kernighan & Ritchie" (-kr):
INDENTARGS="-kr
	-bad
	-c3
	-cd41
	-i8
	-l80
	-ncs
	-psl
	-sob
	-ss
	-ts8
	-blf
	-il0
"

# check if indent(1) is available
command -v indent >/dev/null 2>&1 && INDENT="indent"
command -v gindent >/dev/null 2>&1 && INDENT="gindent"
command -v gnuindent >/dev/null 2>&1 && INDENT="gnuindent"

if [ -z "$INDENT" ]; then
	echo "Error: GNU \"indent\" not found!"
	exit 1
fi

# shellcheck disable=SC2086
$INDENT -v $INDENTARGS "$@"

# -eof-
