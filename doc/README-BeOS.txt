
                     ngIRCd - Next Generation IRC Server

                      (c)2001-2003 by Alexander Barton,
                    alex@barton.de, http://www.barton.de/

               ngIRCd is free software and published under the
                   terms of the GNU General Public License.

                             -- README-BeOS.txt --


      +-------------------------------------------------------------+
      | This text is only available in german at the moment, sorry! |
      | Contributors for this text or the BeOS port are welcome :-) |
      +-------------------------------------------------------------+


BeOS gehoert im Moment (noch?) nicht zu den offiziell unterstuetzten Plat-
formen: der ngIRCd enthaelt zwar bereits einige Anpassungen an BeOS und
compiliert auch, jedoch bricht er bei jedem Connect-Versuch eines Clients
mit diesem Fehler ab:

   select(): Bad file descriptor!

Es sieht leider so aus, als ob das select() von BeOS nicht mit File-Handles
von Pipes verschiedener Prozesse umgehen kann: sobald der Resolver asynchron
gestartet wird, also Pipe-Handles im select() vorhanden sind, fuehrt das zu
obiger Meldung.

Theoretische "Loesung"/Workaround:
Den Resolver unter BeOS nicht verwenden, sondern mit IP-Adressen arbeiten.
Nachteil: der ngIRCd koennte sich nicht zu Servern verbinden, die dynamische
Adressen benutzen -- dazu muesste er den Namen aufloesen. Ansonsten sollte
es eigentlich zu keinen Beeintraechtigungen kommen ...

Also: wenn es jemand implementieren will ... ;-))

Vielleicht mache ich es auch irgendwann mal selber. Mal sehen.

2002-05-19:
Ich habe gerade damit ein wenig gespielt und den Source hier so geaendert,
dass unter BeOS keine Resolver-Subprozesse mehr erzeugt werden, sondern mit
den "rohen" IP-Adressen gearbeitet wird. Das funktioniert so weit auch,
allerdings verschluckt sich BeOS nun bei anderen Funktionen, so zum Beispiel
bei close(), wenn ein Socket eines Clients geschlossen werden soll!?
Sehr komisch.
Wer Interesse daran hat, das weiter zu verfolgen, der moege sich bitte mit
mir in Verbindung setzen (alex@barton.de), ich maile gerne meine Patches zu.
Fuer eine Aenderung im CVS ist es aber meiner Meinung nach noch zu frueh ...

-- 
$Id: README-BeOS.txt,v 1.7 2003/05/15 21:47:57 alex Exp $
