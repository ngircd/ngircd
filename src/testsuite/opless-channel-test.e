# ngIRCd test suite
# Op-less channel test

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

send "JOIN +Channel\r"
expect {
       timeout { exit 1 }
       "@* JOIN :+Channel"
}

send "mode +Channel +t\r"
expect {
       timeout { exit 1 }
       "477"
}

send "quit\r"
expect {
       timeout { exit 1 }
       "ERROR :Closing connection"
}
