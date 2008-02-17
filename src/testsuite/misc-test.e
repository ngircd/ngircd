# $Id: misc-test.e,v 1.2 2008/02/17 13:51:00 alex Exp $

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

send "summon\r"
expect {
	timeout { exit 1 }
	"445"
}

send "users\r"
expect {
	timeout { exit 1 }
	"446"
}

send "info\r"
expect {
	timeout { exit 1 }
	"371"
}
expect {
	timeout { exit 1 }
	"374"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR"
}

# -eof-
