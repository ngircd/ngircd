
                    ngIRCd - Next Generation IRC Server

                      (c)2001-2005 Alexander Barton,
                   alex@barton.de, http://www.barton.de/

               ngIRCd is free software and published under the
                  terms of the GNU General Public License.


                           -- README-AUX.txt --


Since version 0.2.2-pre Apple's A/UX belongs to the officially supported 
platforms. It is not restricted in any way.

Since version 0.5.0 ngIRCd's source compiles with the native A/UX c 
compiler. GNU C isn't a must-have anymore.

The following software packages are needed:

 - GNU sed
   Source:
   http://www.rezepte-im-web.de/appleux/sed-3.02.tar.gz
   http://arthur.barton.de/pub/unix/aux/tools/sed-3.02.tar.gz

   A/UX comes with /bin/sed which isn't supporting all functions needed
   by GNU automake/autoconf.

   Warning: When installing GNU sed please make sure that A/UX doesn't
   use the old one anymore which means set the $PATH or replace /bin/sed
   at all.

 - libUTIL.a
   Source:
   ftp://ftp.mayn.de/pub/really_old_stuff/apple/apple_unix/Sys_stuff/libUTIL-2.1.tar.gz>
   http://arthur.barton.de/pub/unix/aux/libraries/libUTIL-2.1.tar.gz

   This library contains functions that are common on other UNIX
   systems but not on A/UX e.g. memmove(), strerror() and strdup().


After installation of these packages just do a "./configure" and "make" to
compile ngIRCd on A/UX.


A few hints in case of errors:

 - Either there's an 'install' on your system which is completely broken
   (so 'configure' uses its own shell script) or use a fully functionable one.
   There's at least one binary "out there" causing problems. The one
   of the GNU fileutils works fine:
   http://arthur.barton.de/pub/unix/aux/tools/fileutils-4.0.tar.gz

 - The precompiled binary of the old 'bash' shouldn't be installed within
   /bin (better do this in /usr/local/bin) because 'configure' would
   choose it as its shell which wouldn't work.

 - Because of limitations of /bin/sh on A/UX it can't be used to create
   the 'config.status' script. Better rename /bin/sh to /bin/sh.AUX and
   replace it by a symbolic link to /bin/ksh (ln -s /bin/ksh /bin/sh as
   root).
   These procedure shouldn't cause you into problems and is recommended
   even if you don't use ngIRCd.

-- 
$Id: README-AUX.txt,v 1.10 2006/07/23 12:19:57 alex Exp $
