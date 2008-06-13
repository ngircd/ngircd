spawn telnet localhost 6789
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

send "privmsg nick :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}

send "privmsg nick\r"
expect {
	timeout { exit 1 }
	"412"
}

send "privmsg\r"
expect {
	timeout { exit 1 }
	"411"
}

send "privmsg nick,nick :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test\r*@* PRIVMSG nick :test"
}

send "privmsg nick,#testChannel,nick :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test\r*401*@* PRIVMSG nick :test"
}

send "privmsg doesnotexist :test\r"
expect {
	timeout { exit 1 }
	"401"
}

send "privmsg ~user@ngircd.test.server :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}


send "privmsg ~user\%localhost :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}

send "privmsg nick!~user@localhost :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}

send "away :away\r"
expect {
	timeout { exit 1 }
	"306"
}

send "privmsg nick :test\r"
expect {
	timeout { exit 1 }
	"301"
}

send "away\r"
expect {
	timeout { exit 1 }
	"305"
}

send "privmsg \$ngircd.test.server :test\r"
expect {
	timeout { exit 1 }
	"481"
}

send "privmsg #*.de :test\r"
expect {
	timeout { exit 1 }
	"481"
}

send "oper TestOp 123\r"

send "privmsg \$ngircd.test.server :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}

send "privmsg \$*.test*.server :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}

send "privmsg \$noDotServer :test\r"
expect {
	timeout { exit 1 }
	"401"
}

#cannot test host mask since localhost has no '.' as RFC requires

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

# -eof-
