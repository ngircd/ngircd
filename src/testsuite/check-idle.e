# $Id: check-idle.e,v 1.1.8.1 2004/09/04 20:49:36 alex Exp $

spawn telnet localhost 6789
expect {
	timeout { exit 1 }
	"Connected"
}

send "nick IdleTest\r"
send "user idle . . :Idle-Test\r"
expect {
	timeout { exit 1 }
	"433 * IdleTest :Nickname already in use" { exit 99 }
	"376"
}

send "lusers\r"
expect {
	timeout { exit 1 }
	"251 IdleTest :There are 1 users and 0 services on 1 servers" { set r 0 }
	"251 IdleTest :There are" { set r 99 }
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

exit $r

# -eof-
