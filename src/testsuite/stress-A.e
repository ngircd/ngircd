# $Id: stress-A.e,v 1.1 2002/09/09 22:56:07 alex Exp $

spawn telnet localhost 6789
expect {
	timeout { exit 1 }
	"Connected"
}

# -eof-
