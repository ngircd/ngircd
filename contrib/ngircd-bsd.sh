#!/bin/sh

# PROVIDE: ngircd
# REQUIRE: NETWORKING SERVERS
# BEFORE: DAEMON
# KEYWORD: FreeBSD shutdown

# Add the following line to /etc/rc.conf to enable `ngircd':
#
#ngircd_enable="YES"
#

. "/etc/rc.subr"

name="ngircd"
rcvar=`set_rcvar`

command="/usr/local/sbin/ngircd"
command_args=""

load_rc_config "$name"
: ${ngircd_enable="NO"}
: ${ngircd_flags=""}

required_files="/usr/local/etc/$name.conf"
pidfile="${ngircd_pidfile:-/var/run/${name}/${name}.pid}"

if [ ! x"${ngircd_chrootdir}" = x ];then
	# Mount a devfs in the chroot directory if needed
	if [ ! -c ${ngircd_chrootdir}/dev/random \
	  -o ! -c ${ngircd_chrootdir}/dev/null ]; then
		umount ${ngircd_chrootdir}/dev 2>/dev/null
		mount_devfs devfs ${ngircd_chrootdir}/dev
	fi

	devfs -m ${ngircd_chrootdir}/dev rule apply hide
	devfs -m ${ngircd_chrootdir}/dev rule apply path null unhide
	devfs -m ${ngircd_chrootdir}/dev rule apply path random unhide

	# Copy local timezone information if it is not up to date.
	if [ -f /etc/localtime ]; then
		cmp -s /etc/localtime "${named_chrootdir}/etc/localtime" || \
			cp -p /etc/localtime "${named_chrootdir}/etc/localtime"
	fi

	pidfile="${ngircd_chrootdir}${pidfile}"
fi

run_rc_command "$1"

# -eof-
