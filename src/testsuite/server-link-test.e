# ngIRCd test suite
# server-server link test

spawn telnet 127.0.0.1 6790
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

send "version ngircd.test.server2\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server2 351"
}
send "version ngircd.test.server\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 351"
}

send "whois ngircd.test.server nick\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 318"
}

send "admin ngircd.test.server\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 259 nick :admin@irc.server"
}

send "links\r"
expect {
	timeout { exit 1 }
	"364 nick ngircd.test.server ngircd.test.server2 :1"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
