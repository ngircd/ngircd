
                     ngIRCd - Next Generation IRC Server

                      (c)2001,2002 by Alexander Barton,
                    alex@barton.de, http://www.barton.de/

                  ngIRCd ist freie Software und steht unter
                       der GNU General Public License.

                             -- README-AUX.txt --


Seit Version 0.2.2-pre gehoert Apple A/UX zu den offiziell unterstuetzten
Plattformen. Er ist im vollen Funktionsumfang nutzbar.

Ab Version 0.5.0 compiliert zudem der ngIRCd mit dem nativen A/UX-Compiler,
d.h. GNU C wird nicht mehr zwingend vorausgesetzt.

Folgende Software wird jedoch benoetigt:

  - GNU sed
    Bezugsquellen:
    http://www.rezepte-im-web.de/appleux/sed-3.02.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Tools/sed-3.02.tar.gz

    A/UX beinhaltet ein /bin/sed, dieses unterstuetzt jedoch leider nicht
    alle Funktionen, die GNU automake/autoconf nutzen.
    Achtung: bitte bei der Installation von GNU sed sicherstellen, dass
    immer dieses und nie das von A/UX verwendet wird (also $PATH entsprechend
    anpassen bzw. die A/UX-Version komplett ersetzen)!

  - libUTIL.a
    Bezugsquellen:
    http://ftp.mayn.de/pub/apple/apple_unix/Sys_stuff/libUTIL-2.1.tar.gz
    ftp://arthur.ath.cx/pub/AUX/Software/Libraries/libUTIL-2.1.tar.gz

    Diese Library beinhaltet Systemfunktionen, die auf UNIXoiden Systemen
    gaengig, unter A/UX jedoch leider nicht verfuegbar sind. Dazu gehoert
    u.a. memmove(), strerror() und strdup().

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

  - da die /bin/sh von A/UX recht limitiert ist, kann sie u.a. nicht zum
    Erzeugen des "config.status"-Scripts verwendet werden.
    Abhilfe: /bin/sh umbenennen (z.B. in "/bin/sh.AUX") und durch einen (am
    besten symbolischen) Link auf /bin/ksh ersetzen.
    Dieser Schritt sollte keine Probleme nach sich ziehen und ist daher immer,
    auch unabhaengig vom ngIRCd, empfehlenswert.


-- 
$Id: README-AUX.txt,v 1.4 2002/11/11 00:59:11 alex Exp $
