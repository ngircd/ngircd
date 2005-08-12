# $Id: stress-A.e,v 1.2 2005/08/12 21:35:12 alex Exp $

set timeout 30

spawn telnet localhost 6789
expect {
	timeout { exit 1 }
	"Connected"
}

# -eof-
