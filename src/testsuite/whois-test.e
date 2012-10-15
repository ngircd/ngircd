# ngIRCd test suite
# WHOIS test

spawn telnet localhost 6789
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
	"311 nick nick ~user localhost* \* :Real Name\r"
}
expect {
	timeout { exit 1 }
	"318 nick nick :"
}

send "whois *\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user localhost* \* :Real Name\r"
}

send "whois n*\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user localhost* \* :Real Name\r"
}

send "whois ?ick\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user localhost* \* :Real Name\r"
}

send "whois ????,n?*k\r"
expect {
	timeout { exit 1 }
	"311 nick nick ~user localhost* \* :Real Name\r"
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

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR"
}

# -eof-
