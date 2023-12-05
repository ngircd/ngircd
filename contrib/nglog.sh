#!/bin/bash
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2020 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# This script parses the log output of ngircd(8), and colorizes the messages
# according to their log level. Example usage:
# ./src/ngircd/ngircd -f $PWD/doc/sample-ngircd.conf -np | ./contrib/nglog.sh
#

gawk '
  /^\[[[:digit:]]+:0 / {print "\033[1;95m" $0 "\033[0m"}
  /^\[[[:digit:]]+:1 / {print "\033[1;35m" $0 "\033[0m"}
  /^\[[[:digit:]]+:2 / {print "\033[1;91m" $0 "\033[0m"}
  /^\[[[:digit:]]+:3 / {print "\033[1;31m" $0 "\033[0m"}
  /^\[[[:digit:]]+:4 / {print "\033[1;33m" $0 "\033[0m"}
  /^\[[[:digit:]]+:5 / {print "\033[1m" $0 "\033[0m"}
  /^\[[[:digit:]]+:6 / {print $0}
  /^\[[[:digit:]]+:7 / {print "\033[90m" $0 "\033[0m"}
' </dev/stdin &

wait
