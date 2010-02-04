
                    ngIRCd - Next Generation IRC Server

                      (c)2001-2010 Alexander Barton,
                   alex@barton.de, http://www.barton.de/

               ngIRCd is free software and published under the
                  terms of the GNU General Public License.


                         -- README-Interix.txt --


ngIRCd release 15 has successfully been tested on Microsoft Windows XP
Professional using the Services for UNIX (SFU) version 3.5 and Microsoft
Windows 7 with the bundled Subsystem for UNIX Applications (SUA).

SFU are supported on Windows 2000, Windows 2000 Server, Windows XP, and
Windows Server 2003. SUA is supported on Windows Server 2003 R2, Windows
Server 2008 & 2008 R2, Windows Vista, and Windows 7 -- so ngIRCd should be
able to run on all of these platforms.

But please note that the poll() API function is not fully implemented by
SFU/SUA and therefore can't be used by ngIRCd -- which normally would be
the default. Please see <http://www.suacommunity.com/faqs.aspx> section
4.25 for details:

  "If you do try to use the poll() API your program will block on the
  API call forever. You must direct your program to build using the
  select() API."

So when running the ./configure script, you HAVE TO DISABLE poll() support:

  ./configure --without-poll

ngIRCd then defaults to using the select() API function which works fine.

