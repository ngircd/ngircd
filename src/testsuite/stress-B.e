# $Id: stress-B.e,v 1.2 2005/08/12 21:38:52 alex Exp $

send "user user . . :User\r"
expect {
	timeout { exit 1 }
	"376"
}

sleep 2

send "oper TestOp 123\r"
expect {
	timeout { exit 1 }
	"MODE test* :+o"
}
expect {
	timeout { exit 1 }
	"381 test*"
}

sleep 2

send "join #channel\r"
expect {
	timeout { exit 1 }
	":test*!~user@* JOIN :#channel"
}
expect {
	timeout { exit 1 }
	"366"
}

send "mode #channel\r"
expect {
	timeout { exit 1 }
	"324 test* #channel"
}

send "join #channel2\r"
expect {
	timeout { exit 1 }
	":test*!~user@* JOIN :#channel2"
}
expect {
	timeout { exit 1 }
	"366"
}

send "names\r"
expect {
	timeout { exit 1 }
	"366"
}

sleep 3

send "part #channel2\r"
expect {
	timeout { exit 1 }
	":test*!~user@* PART #channel2"
}

send "part #channel\r"
expect {
	timeout { exit 1 }
	":test*!~user@* PART #channel"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

# -eof-
