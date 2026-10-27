# ngIRCd test suite
# PRIVMSG and NOTICE test

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
	"@* PRIVMSG nick :test"
}

send "privmsg ,,,, :dummy\r"
send "privmsg ,,,nick,,&server,,, :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}
expect {
	timeout { exit 1 }
	"404"
}

send "privmsg Nick,#testChannel,nick :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test\r*401"
}

send "privmsg doesnotexist :test\r"
expect {
	timeout { exit 1 }
	"401"
}

send "privmsg ~UsEr@ngIRCd.Test.Server :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}

send "mode nick +b\r"
expect {
	timeout { exit 1 }
	"MODE nick :+b"
}
send "privmsg nick :test\r"
expect {
	timeout { exit 1 }
	"486"
}
send "mode nick -b\r"
expect {
	timeout { exit 1 }
	"MODE nick :-b"
}

send "privmsg ~user\%127.0.0.1 :test\r"
expect {
	timeout { exit 1 }
	"@* PRIVMSG nick :test"
}

send "privmsg Nick!~User@127.0.0.1 :test\r"
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

send "privmsg \$*.t* :test\r"
expect {
	timeout { exit 1 }
	"414 nick \$*.t* :Wildcard in toplevel domain"
}

send "privmsg \$noDotServer :test\r"
expect {
	timeout { exit 1 }
	"401"
}

# Too many targets: use another connection, as each target adds one
# second of penalty time, which delays all further commands.
set main $spawn_id
spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick targets\r"
send "user user . . :User\r"
expect {
	timeout { exit 1 }
	"376"
}
send "privmsg n0,n1,n2,n3,n4,n5,n6,n7,n8,n9,n10,n11,n12,n13,n14,n15,n16,n17,n18,n19,n20,n21,n22,n23,n24,n25 :test\r"
expect {
	timeout { exit 1 }
	"407 targets n25 :Too many recipients"
}
close
wait
set spawn_id $main

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
