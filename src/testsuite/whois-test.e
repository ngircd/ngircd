# ngIRCd test suite
# WHOIS test

spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}

send "nick nick\r"
send "user user . . :Real Name\r"
expect {
	timeout { exit 1 }
	"376"
}

send "whois nick\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user 127.0.0.1 \* :Real Name\r"
}
expect {
	timeout { exit 1 }
	"318 nick nick :"
}

send "whois *\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user 127.0.0.1* \* :Real Name\r"
}

send "whois n*\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user 127.0.0.1* \* :Real Name\r"
}

send "whois ?ick\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user 127.0.0.1* \* :Real Name\r"
}

send "whois ????,n?*k\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user 127.0.0.1* \* :Real Name\r"
}

send "whois unknown\r"
expect {
	timeout { exit 1 }
	"401 nick unknown :"
}
expect {
	timeout { exit 1 }
	"318 nick unknown :"
}

send "whois ngircd.test.server2 nick\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server2 311 nick nick ~user 127.0.0.1* \* :Real Name\r"
}

send "whois nosuchserver unknown\r"
expect {
	timeout { exit 1 }
	"402 nick nosuchserver :"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
