# [ngIRCd](https://ngircd.barton.de) - Supplemental Files

This `contrib/` directory contains the following sub-folders and files:

- `Debian/` folder: This subfolder contains the _rules_ file and additional
  assets for building Debian packages.

- `de.barton.ngircd.metainfo.xml`: AppStream metadata file.

- `de.barton.ngircd.plist[.tmpl]`: launchd(8) property list file.

- `Dockerfile`: Container definition file, for Docker or Podman for example.
  More information can be found in the `doc/Container.md` file.

- `ngindent.sh`: Script to indent the code of ngIRCd in the "standard way".

- `ngircd-bsd.sh`: Start/stop script for FreeBSD.

- `ngircd-fail2ban.conf`: fail2ban(1) filter configuration for ngIRCd.

- `ngircd-redhat.init`: Start/stop script for old(er) RedHat-based
  distributions (like CentOS and Fedora), which did _not_ use systemd(8).

- `ngIRCd-Logo.gif`: The ngIRCd logo as GIF file.

- `ngircd.logcheck`: Sample rules for logcheck(8) to ignore "normal" log
  messages of ngIRCd.

- `ngircd.service`: systemd(8) service unit configuration file.

- `ngircd.socket`: systemd(8) socket unit configuration file for "socket
  activation".

- `ngircd.spec`: RPM "spec" file.

- `nglog.sh`: Script for colorizing the log messages of ngircd(8) according to
  their log level. Example: `./src/ngircd/ngircd -n | ./contrib/nglog.sh`.

- `platformtest.sh`: Build ngIRCd and output a "result line" suitable for
  the `doc/Platforms.txt` file.
