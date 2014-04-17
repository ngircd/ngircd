# ngIRCd test suite
# Server connect test

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
