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
# $Id: wait-tests.sh,v 1.1 2004/09/04 15:44:45 alex Exp $
#

[ "$1" -gt 0 ] 2> /dev/null && MAX="$1" || MAX=5

msg=0
while true; do
  count=`ps | grep "expect " | wc -l`
  count=`expr $count - 1`

  [ $count -le $MAX ] && break

  if [ $msg -lt 1 ]; then
    echo -n "      waiting for test scripts to settle: "
    msg=1
  fi

  # there are still clients connected. Wait ...
  echo -n "$count>$MAX "
  sleep 1
done

[ $msg -gt 0 ] && echo "done: $count"
exit 0

# -eof-
