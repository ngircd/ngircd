# ngIRCd test suite
# Idle test

spawn telnet 127.0.0.1 6789
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
	"ERROR :Closing connection"
}

exit $r
