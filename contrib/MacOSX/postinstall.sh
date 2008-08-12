#!/bin/sh
# ngIRCd Mac OS X postinstall/postupgrade script

LDPLIST="/Library/LaunchDaemons/de.barton.ngircd.plist"

if [ ! -e /etc/ngircd ]; then
	echo "Creating symlink: /opt/ngircd/etc -> /etc/ngircd"
	ln -s /opt/ngircd/etc /etc/ngircd || exit 1
else
	echo "/etc/ngircd already exists. Don't create symlink."
fi

if [ ! -e /opt/ngircd/etc/ngircd.conf ]; then
	echo "Creating default configuration: /opt/ngircd/etc/ngircd.conf"
	cp /opt/ngircd/share/doc/ngircd/sample-ngircd.conf \
	 /opt/ngircd/etc/ngircd.conf || exit 1
else
	echo "/opt/ngircd/etc/ngircd.conf exists. Don't copy sample file."
fi
chmod o-rwx /opt/ngircd/etc/ngircd.conf

if [ -f "$LDPLIST" ]; then
	echo "Fixing ownership and permissions of LaunchDaemon script ..."
	chown root:wheel "$LDPLIST" || exit 1
	chmod 644 "$LDPLIST" || exit 1
fi

if [ -f /tmp/ngircd_needs_restart ]; then
	echo "ngIRCd should be (re-)started ..."
	if [ -r "$LDPLIST" ]; then
		echo "LaunchDaemon script found, starting daemon ..."
		launchctl load -w "$LDPLIST" || exit 1
		echo "OK, LaunchDaemon script loaded successfully."
	else
		echo "LaunchDaemon script not installed. Can't start daemon."
	fi
else
	echo "Not loading LaunchDaemon script."
fi
rm -f /tmp/ngircd_needs_restart

# -eof-
