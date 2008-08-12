#!/bin/sh
# ngIRCd Mac OS X preinstall/preupgrade script

LDPLIST="/Library/LaunchDaemons/de.barton.ngircd.plist"

rm -f /tmp/ngircd_needs_restart || exit 1
if [ -r "$LDPLIST" ]; then
	echo "LaunchDaemon script found, checking status ..."
	launchctl list | fgrep "de.barton.ngIRCd" >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		# ngIRCd is already running; stop it and touch a
		# "stamp file" so that we know that we have to
		# restart it after installation/upgrade.
		echo "ngIRCd is already running; stop it ..."
		launchctl unload "$LDPLIST" || exit 1
		echo "Daemon has been stopped."
		touch /tmp/ngircd_needs_restart || exit 1
	else
		echo "ngIRCd is not running."
	fi
else
	echo "LaunchDaemon script not found."
fi

# -eof-
