
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
    ftp://arthur.ath.cx/pub/AUX/Software/Development/gcc-2.8.1-auxbin.tar.gz

  - GNU make
    Bezugsquellen:
    http://www.rezepte-im-web.de/appleux/make-3.79.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Development/make-3.79.tar.gz

  - GNU sed
    Bezugsquellen:
    http://www.rezepte-im-web.de/appleux/sed-3.02.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Tools/sed-3.02.tar.gz

  - install (z.B. aus den GNU fileutils)
    Ein install, welches entweder so "broken" ist, dass configure das eigene
    Shell-Script waehlt, oder eines, das funktioniert. Leider ist mindestens
    ein Binary im Umlauf, das Probleme macht.
    Bezugsquelle:
    ftp://arthur.ath.cx/pub/UNIX/AUX/Software/Tools/fileutils-4.0.tar.gz

  - libUTIL.a
    Bezugsquellen:
    http://ftp.mayn.de/pub/apple/apple_unix/Sys_stuff/libUTIL-2.1.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Libraries/libUTIL-2.1.tar.gz

Nachdem diese Pakete entsprechend installiert sind, reicht ein ganz normales
"./configure" und "make" aus, um den ngIRCd unter A/UX zu compilieren.


Noch ein paar Hinweise, wenn es doch (noch) nicht klappt:

  - auf dem System muss entweder ein install vorhanden sein, welches so
    "broken" ist, dass configure das eigene Shell-Skript waehlt, oder eben
    eines, welches funktioniert. Leider ist mindestens ein Binary im Um-
    lauf, welches Probleme verursacht. Das Binary aus folgenden GNU
    fileutils funktioniert hier aber z.B.:
    ftp://arthur.ath.cx/pub/UNIX/AUX/Software/Tools/fileutils-4.0.tar.gz

  - das sich im Umlauf befindende vorcompilierte Binary der alten Bash sollte
    unbedingt ausserhalb von /bin (z.B. unter /usr/local/bin) installiert
    werden. Ansonsten waehlt es das configure-Script als Shell aus, leider
    funktioniert das aber nicht.
    Das config.status-Script sollte mit der ksh als Interpreter erstellt
    worden sein (siehe erste Zeile davon!).


Hier die Zeiten von Alex System (Macintosh SE/30, 32 MB, A/UX 3.0.1):
configure: 7:33, make: 12:02


-- 
$Id: README-AUX.txt,v 1.3 2002/04/29 14:19:48 alex Exp $
