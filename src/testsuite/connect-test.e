# $Id: connect-test.e,v 1.1 2002/09/09 10:16:24 alex Exp $

spawn telnet localhost 6789
expect {
	timeout { exit 1 }
	"Connected"
}

send "oper\r"
expect {
	timeout { exit 1 }
	"451"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

# -eof-
