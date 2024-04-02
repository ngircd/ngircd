# ngIRCd test suite
# JOIN test

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

send "JOIN\r"
expect {
       timeout { exit 1}
       "461"
}

send "JOIN #InviteChannel\r"
expect {
       timeout { exit 1 }
       "473"
}

send "JOIN #FullKeyed\r"
expect {
       timeout { exit 1 }
       "475"
}

send "JOIN #FullKeyed WrongKey\r"
expect {
       timeout { exit 1 }
       "475"
}

send "JOIN #FullKeyed Secret\r"
expect {
       timeout { exit 1 }
       "471"
}

send "JOIN #TopicChannel\r"
expect {
       timeout { exit 1 }
       "@* JOIN :#TopicChannel"
}
expect {
       timeout { exit 1 }
       "332"
}

send "JOIN 0\r"
send "JOIN #1,#2,#3,#4\r"
send "JOIN #5\r"
expect {
       timeout { exit 1 }
       "405"
}
send "JOIN 0\r"

send "JoIn #MultiMode\r"
expect {
       timeout { exit 1 }
       "474 nick #MultiMode"
}

send "OPer TestOp 123\r"
expect {
	timeout { exit 1 }
	"381"
}

send "Mode #MultiMode -b nick!~user\r"
expect {
       timeout { exit 1 }
	"MODE #MultiMode -b nick!~user@*"
}

send "jOiN #MULTIMODE\r"
expect {
       timeout { exit 1 }
       "@* JOIN :#MULTIMODE"
}
expect {
       timeout { exit 1 }
       "366"
}
send "ModE #MULTImode\r"
expect {
       timeout { exit 1 }
       "324 nick #MultiMode +Pnt"
}
send "mODe #multimode +b\r"
expect {
       timeout { exit 1 }
       "367 nick #MultiMode banned!~ghost@example.com ngircd.test.server"
}
expect {
       timeout { exit 1 }
       "368 nick #MultiMode"
}

send "quit\r"
expect {
       timeout { exit 1 }
       "ERROR :Closing connection"
}
