#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
#
# Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
# der GNU General Public License (GPL), wie von der Free Software Foundation
# herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
# der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
# Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
# der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
#

.tmpl:
	sed \
	    -e s@:ETCDIR:@${sysconfdir}@ \
	    <$< >$@

SUFFIXES = .tmpl

static_docs = FAQ.txt GIT.txt Protocol.txt Platforms.txt README-AUX.txt \
	README-BeOS.txt RFC.txt Services.txt SSL.txt Zeroconf.txt \
	HowToRelease.txt

doc_templates = sample-ngircd.conf.tmpl

generated_docs = sample-ngircd.conf

toplevel_docs = ../AUTHORS ../COPYING ../ChangeLog ../INSTALL ../NEWS ../README

SUBDIRS = src

EXTRA_DIST = $(static_docs) $(doc_templates)

CLEANFILES = $(generated_docs)

maintainer-clean-local:
	rm -f Makefile Makefile.in

all: $(generated_docs)

install-data-hook: $(static_docs) $(toplevel_docs) $(generated_docs)
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	if [ ! -f $(DESTDIR)$(sysconfdir)/ngircd.conf ]; then \
	  $(INSTALL) -m 600 -c sample-ngircd.conf $(DESTDIR)$(sysconfdir)/ngircd.conf; \
	 fi
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	for f in $(static_docs) $(toplevel_docs); do \
	  $(INSTALL) -m 644 -c $(srcdir)/$$f $(DESTDIR)$(docdir)/; \
	 done
	for f in $(generated_docs); do \
	  $(INSTALL) -m 644 -c $$f $(DESTDIR)$(docdir)/; \
	 done

uninstall-hook:
	rm -rf $(DESTDIR)$(docdir)

srcdoc:
	make -C src srcdoc

# -eof-
