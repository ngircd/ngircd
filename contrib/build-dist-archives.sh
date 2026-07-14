#!/bin/sh
#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2026 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

# This script is meant to be run inside of a Debian "Bookworm" (12) container,
# the latest Debian release which includes a GNU automake 1.11 package that
# still supports "de-ANSI-fication", to build all "usual" distribution archives
# of ngIRCd in "tar.gz", "tar.Z" and "tar.xz" formats. For example like this:
#
#   podman run --rm -v "$PWD":/src debian:bookworm-slim \
#     /src/contrib/build-dist-archives.sh
#
# The optional first parameter is the path to the ngIRCd source tree inside of
# the container (default: "/src"). Please note that the build runs in place
# and overwrites all generated files (./configure, Makefiles, ...) there!
#
# Once Debian "Bookworm" has been moved to "archive.debian.org", "apt-get
# update" will fail and the APT sources of the container must be adjusted.

set -ex
cd "${1:-/src}"
test -e src/ngircd/ngircd.c
apt-get update
apt-get install --no-install-recommends -y \
	automake1.11 \
	expect \
	gcc \
	git \
	libc6-dev \
	make \
	ncompress \
	pkg-config \
	telnet \
	xz-utils
# The version number is derived from "git describe", so make sure that Git
# works even when the source tree is owned by a different user.
git config --global --add safe.directory "$PWD"
git describe
./autogen.sh
./configure
make -j2 distcheck
make dist-tarZ
make dist-xz
ls -hl ngircd-*.tar.*
