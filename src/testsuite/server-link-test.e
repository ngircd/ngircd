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

send "list #TopicChannel,+ModelessChannel ngircd.test.server\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 322 nick #TopicChannel "
}
expect {
	timeout { exit 1 }
	":ngircd.test.server 322 nick +ModelessChannel "
}

# Changes of lists of local channels must stay local
set link $spawn_id

spawn telnet 127.0.0.1 6789
set local1 $spawn_id
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick local1\r"
send "user user . . :User\r"
expect {
	timeout { exit 1 }
	"376"
}

spawn telnet 127.0.0.1 6790
set local2 $spawn_id
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick local2\r"
send "user user . . :User\r"
expect {
	timeout { exit 1 }
	"376"
}

spawn telnet 127.0.0.1 6789
set local3 $spawn_id
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick local3\r"
send "user user . . :User\r"
expect {
	timeout { exit 1 }
	"376"
}

send -i $local1 "join &local\r"
expect {
	-i $local1
	timeout { exit 1 }
	"366"
}
send -i $local1 "mode &local +i\r"
expect {
	-i $local1
	timeout { exit 1 }
	"@* MODE &local +i\r"
}

send -i $local2 "join &local\r"
expect {
	-i $local2
	timeout { exit 1 }
	"366"
}
send -i $local2 "mode &local +I local3\r"
expect {
	-i $local2
	timeout { exit 1 }
	"@* MODE &local +I local3!*@*"
}
send -i $local2 "version ngircd.test.server\r"
expect {
	-i $local2
	timeout { exit 1 }
	":ngircd.test.server 351 local2 "
}

send -i $local3 "join &local\r"
expect {
	-i $local3
	timeout { exit 1 }
	"473 local3 &local"
}

foreach id [list $local1 $local2 $local3] {
	send -i $id "quit\r"
	expect {
		-i $id
		timeout { exit 1 }
		"ERROR :Closing connection"
	}
}

set spawn_id $link
send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
