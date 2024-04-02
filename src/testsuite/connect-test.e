# ngIRCd test suite
# Server connect test

spawn telnet 127.0.0.1 6789
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
	"ERROR :Closing connection"
}
