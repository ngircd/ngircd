# ngIRCd test suite
# server-server login test

spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}

# Register server
send "PASS pwd1 0210-IRC+ ngIRCd|testsuite0:CHLMSX P\r"
send "SERVER ngircd.test.server3 :Testsuite Server Emulation\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server PASS pwd3 0210-IRC+ ngIRCd|"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server SERVER ngircd.test.server 1 :"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server 005 "
}
expect {
	timeout { exit 1 }
	":ngircd.test.server 376 "
}

# End of handshake
send ":ngircd.test.server3 376 ngircd.test.server :End of MOTD command\r"

# Receive existing channels
expect {
	timeout { exit 1 }
	":ngircd.test.server CHANINFO +ModelessChannel +P :A modeless Channel"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server CHANINFO #SecretChannel +Ps :A secret Channel"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server CHANINFO #TopicChannel +Pt :the topic"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server CHANINFO #FullKeyed +Pkl Secret 0 :"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server CHANINFO #InviteChannel +Pi"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server PING :ngircd.test.server"
}

# Emulate network burst
send ":ngircd.test.server3 NICK NickName 1 ~User localhost 1 + :Real Name\r"
send ":ngircd.test.server3 NJOIN #Channel :@NickName\r"

# End of burst
send ":ngircd.test.server3 PONG :ngircd.test.server\r"

# Test server-server link ...
send ":ngircd.test.server3 VERSION\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 351 ngircd.test.server3 "
}

# Make sure our test client is still known in the network
send ":ngircd.test.server3 WHOIS NickName\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 311 ngircd.test.server3 NickName ~User localhost * :Real Name"
}
expect {
	timeout { exit 1 }
	":ngircd.test.server 319 ngircd.test.server3 NickName :@#Channel"
}

expect {
	timeout { exit 1 }
	":ngircd.test.server 318 ngircd.test.server3 NickName :"
}

# Logout
send ":ngircd.test.server3 QUIT\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
