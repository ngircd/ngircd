# ngIRCd test suite
# INVITE test

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

send "invite\r"
expect {
	timeout { exit 1 }
	"461"
}

send "invite nick\r"
expect {
	timeout { exit 1 }
	"461"
}

send "invite nick #channel\r"
expect {
	timeout { exit 1 }
	-re "INVITE nick :?#channel"
}
expect {
	timeout { exit 1 }
	-re "341 nick nick :?#channel"
}

send "invite nosuchnic #TopicChannel\r"
expect {
	timeout { exit 1 }
	"401 nick nosuchnic :No such nick or channel name"
}

send "invite nick #TopicChannel\r"
expect {
	timeout { exit 1 }
	"442 nick #TopicChannel :You are not on that channel"
}

send "join #channel\r"
expect {
	timeout { exit 1 }
	-re "JOIN :?#channel"
}

send "invite nick #channel\r"
expect {
	timeout { exit 1 }
	"443 nick nick #channel :is already on channel"
}

send "mode #channel +i\r"
expect {
	timeout { exit 1 }
	"MODE #channel +i"
}

send "mode #channel -o nick\r"
expect {
	timeout { exit 1 }
	"MODE #channel -o nick"
}

send "invite nick #channel\r"
expect {
	timeout { exit 1 }
	"482 nick #channel :You are not channel operator"
	#it would be reasonable to expect 443 here instead
}

send "part #channel\r"
expect {
	timeout { exit 1}
	"@* PART #channel :"
}

send "invite nick :parameter with spaces\r"
expect {
	timeout { exit 1 }
	"INVITE nick :parameter with spaces"
}
expect {
	timeout { exit 1 }
	"341 nick nick :parameter with spaces"
}

send "away message\r"
expect {
	timeout { exit 1 }
	"306 nick :You have been marked as being away"
}

send "INVITE nick #channel\r"
expect {
	timeout { exit 1 }
	-re "301 nick nick :?message"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
