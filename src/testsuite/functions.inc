#!/bin/sh
#
# ngIRCd Test Suite
# Copyright (c)2002-2004 by Alexander Barton (alex@barton.de)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# $Id: functions.inc,v 1.1 2004/09/06 22:04:06 alex Exp $
#

# test how to call echo to get output without newline
echo -n | grep -- -n >/dev/null 2>&1
if [ $? -eq 0 ]; then
	ECHO_N=""; ECHO_C="\c"
else
	ECHO_N="-n"; ECHO_C=""
fi

echo_n()
{
	echo $ECHO_N "$*$ECHO_C"
}

# -eof-
