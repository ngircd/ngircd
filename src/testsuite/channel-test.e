# $Id: channel-test.e,v 1.1 2002/09/09 10:16:24 alex Exp $

spawn telnet localhost 6789
expect {
	timeout { exit 1 }
	"Connected"
}

send "nick nick\r"
send "user user . . :User\r"
expect {
	timeout { exit 1 }
	"376"
}

send "join #channel\r"
expect {
	timeout { exit 1 }
	":nick!~user@* JOIN :#channel"
}
expect {
	timeout { exit 1 }
	"366"
}

send "part #channel\r"
expect {
	timeout { exit 1 }
	":nick!~user@* PART #channel :nick"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

# -eof-
