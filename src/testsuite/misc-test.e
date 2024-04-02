# ngIRCd test suite
# Misc test

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

# RFC 2812 Section 3.4.1

send "motd\r"
expect {
	timeout { exit 1 }
	"375"
}
expect {
	timeout { exit 1 }
	"372"
}
expect {
	timeout { exit 1 }
	"376"
}

send "motd ngircd.test.server\r"
expect {
	timeout { exit 1 }
	"375"
}
expect {
	timeout { exit 1 }
	"372"
}
expect {
	timeout { exit 1 }
	"376"
}

send "motd doesnotexist\r"
expect {
	timeout { exit 1 }
	"402"
# note this is not specified in RFC 2812, but probably should be
}

# RFC 2812 Section 3.4.3

send "version\r"
expect {
	timeout { exit 1 }
	"351"
}

send "version ngircd.test.server\r"
expect {
	timeout { exit 1 }
	"351"
}

send "version doesnotexist\r"
expect {
	timeout { exit 1 }
	"402"
}

# RFC 2812 Section 3.4.6

send "time\r"
expect {
	timeout { exit 1 }
	"391"
}

send "time ngircd.test.server\r"
expect {
	timeout { exit 1 }
	"391"
}

send "time doesnotexist\r"
expect {
	timeout { exit 1 }
	"402"
}

# RFC 2812 Section 3.4.10

send "info\r"
expect {
	timeout { exit 1 }
	"371"
}
expect {
	timeout { exit 1 }
	"374"
}

# RFC 2812 Section 4.5

send "summon\r"
expect {
	timeout { exit 1 }
	"445"
}

# RFC 2812 Section 4.6

send "users\r"
expect {
	timeout { exit 1 }
	"446"
}

# RFC 2812 Section 4.8

send "userhost\r"
expect {
	timeout { exit 1 }
	"461"
}

send "userhost nick\r"
expect {
	timeout { exit 1 }
	-re ":ngircd.test.server 302 nick :?nick=+.*@127.0.0.1"
}

send "userhost doesnotexist\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 302 nick :\r"
}

send "userhost nick doesnotexist nick doesnotexist\r"
expect {
	timeout { exit 1 }
	-re ":ngircd.test.server 302 nick :nick=+.*@127.0.0.1 nick=+.*@127.0.0.1"
}

send "away :testing\r"
expect {
	timeout { exit 1 }
	"306 nick"
}

send "userhost nick nick nick nick nick nick\r"
expect {
	timeout { exit 1 }
	-re ":ngircd.test.server 302 nick :nick=-.*@127.0.0.1 nick=-.*@127.0.0.1 nick=-.*@127.0.0.1 nick=-.*@127.0.0.1 nick=-.*@127.0.0.1\r"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
