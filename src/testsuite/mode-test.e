# $Id: mode-test.e,v 1.7 2008/02/16 11:27:49 fw Exp $

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
	"@* MODE nick :+i"
}

send "mode nick\r"
expect {
	timeout { exit 1 }
	"221 nick +i"
}

send "mode nick -i\r"
expect {
	timeout { exit 1 }
	"@* MODE nick :-i"
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
	"221 nick +o"
}

send "join #channel\r"
expect {
	timeout { exit 1 }
	"@* JOIN :#channel"
}
expect {
	timeout { exit 1 }
	"366"
}

send "mode #channel +tn\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel +tn"
}

send "mode #channel\r"
expect {
	timeout { exit 1 }
	"324 nick #channel +tn"
}

send "mode #channel +v nick\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel +v nick\r"
}

send "mode #channel +I nick1\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel +I nick1!*@*"
}

send "mode #channel +b nick2@domain\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel +b nick2!*@domain"
}

send "mode #channel +I nick3!user\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel +I nick3!user@*"
}

send "mode #channel -vo nick nick\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel -vo nick nick\r"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

# -eof-
