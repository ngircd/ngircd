
                     ngIRCd - Next Generation IRC Server

                      (c)2001,2002 by Alexander Barton,
                    alex@barton.de, http://www.barton.de/

                  ngIRCd ist freie Software und steht unter
                       der GNU General Public License.

                             -- README-BeOS.txt --


BeOS gehoert im Moment (noch?) nicht zu den offiziell unterstuetzten Plat-
formen: der ngIRCd enthaelt zwar bereits einige Anpassungen an BeOS und
compiliert auch, jedoch bricht er bei jedem Connect-Versuch eines Clients
mit diesem Fehler ab:

   select(): Bad file descriptor!

Es sieht leider so aus, als ob das select() von BeOS nicht mit File-Handles
von Pipes verschiedener Prozesse umgehen kann: sobald der Resolver asyncron
gestartet wird, also Pipe-Handles im select() vorhanden sind, fuehrt das zu
obiger Meldung.

Theoretische "Lösung"/Workaround:
Den Resolver unter BeOS nicht verwenden, sondern mit IP-Adressen arbeiten.
Nachteil: der ngIRCd koennte sich nicht zu Servern verbinden, die dynamische
Adressen benutzen -- dazu muesste er den Namen aufloesen. Ansonsten sollte
es eigentlich zu keinen Beeintraechtigungen kommen ...

Also: wenn es jemand implementieren will ... ;-))

Vielleicht mache ich es auch irgendwann mal selber. Mal sehen.

-- 
$Id: README-BeOS.txt,v 1.1 2002/02/25 14:02:32 alex Exp $
