
                     ngIRCd - Next Generation IRC Server

                      (c)2001,2002 by Alexander Barton,
                    alex@barton.de, http://www.barton.de/

                  ngIRCd ist freie Software und steht unter
                       der GNU General Public License.

                             -- README-AUX.txt --


Seit Version 0.2.2-pre gehoert Apple A/UX zu den offiziell unterstuetzten
Platformen. Er ist im vollen Funktionsumfang nutzbar.

Folgende Software wird jedoch benoetigt:

  - GNU C Compiler (gcc)
    Bezugsquellen:
    http://www.rezepte-im-web.de/appleux/gcc281.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Development/gcc281.tar.gz

  - GNU make
    Bezugsquellen:
    http://www.rezepte-im-web.de/appleux/make-3.79.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Development/make-3.79.tar.gz

  - GNU sed
    Bezugsquellen:
    http://www.rezepte-im-web.de/appleux/sed-3.02.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Tools/sed-3.02.tar.gz

  - libUTIL.a
    Bezugsquellen:
    http://ftp.mayn.de/pub/apple/apple_unix/Sys_stuff/libUTIL-2.1.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Libraries/libUTIL-2.1.tar.gz

Nachdem diese Pakete entsprechend installiert sind, reicht ein ganz normales
"./configure" und "make" aus, um den ngIRCd unter A/UX zu compilieren.


Hier die Zeiten von Alex System (Macintosh SE/30, 32 MB, A/UX 3.0.1):
configure: 7:33, make: 12:02


-- 
$Id: README-AUX.txt,v 1.1 2002/02/25 14:02:32 alex Exp $
