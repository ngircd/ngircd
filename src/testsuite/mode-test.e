# $Id: mode-test.e,v 1.2 2002/09/09 21:26:00 alex Exp $

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

send "mode nick +i\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE nick +i"
}

send "mode nick\r"
expect {
	timeout { exit 1 }
	"211 nick +i"
}

send "mode nick -i\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE nick -i"
}

send "oper TestOp 123\r"
expect {
	timeout { exit 1 }
	"MODE nick :+o"
}
expect {
	timeout { exit 1 }
	"381 nick"
}

send "mode nick\r"
expect {
	timeout { exit 1 }
	"211 nick +o"
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

send "mode #channel +tn\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE #channel +tn"
}

send "mode #channel\r"
expect {
	timeout { exit 1 }
	"324 nick #channel +tn"
}

send "mode #channel +v nick\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE #channel +v nick"
}

send "mode #channel +I nick1\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE #channel +I nick1!*@*"
}

send "mode #channel +b nick2@domain\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE #channel +b nick2!*@domain"
}

send "mode #channel +I nick3!user\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE #channel +I nick3!user@*"
}

send "mode #channel -vo nick\r"
expect {
	timeout { exit 1 }
	":nick!~user@* MODE #channel -vo nick"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

# -eof-
