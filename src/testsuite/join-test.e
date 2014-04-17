# ngIRCd test suite
# JOIN test

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

send "quit\r"
expect {
       timeout { exit 1 }
       "Connection closed"
}
