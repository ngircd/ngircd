# ngIRCd test suite
# Channel test

spawn telnet 127.0.0.1 6789
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
	"@* JOIN :#channel"
}
expect {
	timeout { exit 1 }
	"366"
}

send "topic #channel :Test-Topic\r"
expect {
	timeout { exit 1 }
	"@* TOPIC #channel :Test-Topic"
}

send "who #channel\r"
expect {
	timeout { exit 1 }
	"352 nick #channel"
}
expect {
	timeout { exit 1 }
	"* nick H@ :0 User"
}
expect {
	timeout { exit 1 }
	"315 nick #channel"
}

send "names #channel\r"
expect {
	timeout { exit 1 }
	"353 nick = #channel :@nick"
}
expect {
	timeout { exit 1 }
	"366 nick #channel"
}

send "list\r"
expect {
	timeout { exit 1 }
	"322 nick #channel 1 :Test-Topic"
}
expect {
	timeout { exit 1 }
	"323 nick :End of LIST"
}

send "part #channel :bye bye\r"
expect {
	timeout { exit 1 }
	"@* PART #channel :bye bye"
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

send "join #channel2\r"
expect {
	timeout { exit 1 }
	"@* JOIN :#channel2"
}
expect {
	timeout { exit 1 }
	"366"
}

send "join 0\r"
expect {
	timeout { exit 1 }
	"@* PART #channel2 :"
}
expect {
	timeout { exit 1 }
	"@* PART #channel :"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
