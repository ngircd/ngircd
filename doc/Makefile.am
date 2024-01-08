#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

.tmpl:
	$(AM_V_GEN)sed \
	    -e "s@:ETCDIR:@${sysconfdir}@" \
	    -e "s@:DOCDIR:@${docdir}@" \
	    <$< >$@

SUFFIXES = .tmpl

static_docs = \
	Bopm.txt \
	Capabilities.txt \
	Commands.txt \
	Container.md \
	Contributing.txt \
	FAQ.md \
	HowToRelease.txt \
	Modes.txt \
	PAM.txt \
	Platforms.txt \
	Protocol.txt \
	README-AUX.txt \
	README-BeOS.txt \
	README-Interix.txt \
	RFC.txt \
	Services.txt \
	SSL.md

doc_templates = sample-ngircd.conf.tmpl

generated_docs = sample-ngircd.conf

toplevel_docs = ../AUTHORS.md ../COPYING ../ChangeLog ../INSTALL.md ../NEWS ../README.md

SUBDIRS = src

EXTRA_DIST = $(static_docs) $(doc_templates)

CLEANFILES = $(generated_docs)

maintainer-clean-local:
	rm -f Makefile Makefile.in

all: $(generated_docs)

install-data-hook: $(static_docs) $(toplevel_docs) $(generated_docs)
	$(MKDIR_P) -m 755 $(DESTDIR)$(sysconfdir)
	@if [ ! -f $(DESTDIR)$(sysconfdir)/ngircd.conf ]; then \
	  ${MAKE} install-config; \
	 fi
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)
	for f in $(static_docs) $(toplevel_docs); do \
	  $(INSTALL) -m 644 -c $(srcdir)/$$f $(DESTDIR)$(docdir)/; \
	 done
	for f in $(generated_docs); do \
	  $(INSTALL) -m 644 -c $$f $(DESTDIR)$(docdir)/; \
	 done

install-config:
	$(INSTALL) -m 600 -c sample-ngircd.conf $(DESTDIR)$(sysconfdir)/ngircd.conf
	@echo; \
	 echo " ** NOTE: Installed sample configuration file:"; \
	 echo " ** \"$(DESTDIR)$(sysconfdir)/ngircd.conf\""; \
	 echo

uninstall-hook:
	rm -rf $(DESTDIR)$(docdir)
	@if cmp --silent sample-ngircd.conf $(DESTDIR)$(sysconfdir)/ngircd.conf; then \
	  ${MAKE} uninstall-config; \
	 else \
	  echo; \
	  echo " ** NOTE: Not uninstalling changed configuration file:"; \
	  echo " ** \"$(DESTDIR)$(sysconfdir)/ngircd.conf\""; \
	  echo; \
	 fi

uninstall-config:
	rm -f $(DESTDIR)$(sysconfdir)/ngircd.conf

.PHONY: install-config uninstall-config

# -eof-
