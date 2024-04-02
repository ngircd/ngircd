# ngIRCd test suite
# KICK test

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

send "kick #Channel nick\r"
expect {
       timeout { exit 1 }
       "403"
}

send "join #Channel\r"

send "kick #Channel nick\r"
expect {
       timeout { exit 1 }
       "@* KICK #Channel nick :nick"
}

send "join #Channel\r"

send "kick #Channel nick :reason\r"
expect {
       timeout { exit 1 }
       "@* KICK #Channel nick :reason"
}

send "join #Channel,#Channel2\r"

send "kick #Channel,#Channel2 nick\r"
expect {
       timeout { exit 1 }
       "461"
}

send "kick #Channel,#Channel2,#NoExists,#NoExists nick1,nick,nick3,nick :reason\r"
expect {
       timeout { exit 1 }
       "401"
}
expect {
       timeout { exit 1 }
       "@* KICK #Channel2 nick :reason"
}
expect {
       timeout { exit 1 }
       "401"
}
expect {
       timeout { exit 1 }
       "403"
}

send "kick #Channel nick2,nick,nick3\r"
expect {
       timeout { exit 1 }
       "401"
}
expect {
       timeout { exit 1 }
       "@* KICK #Channel nick :nick"
}
expect {
       timeout { exit 1 }
       "401"
}

send "kick #Channel ,,\r"
expect {
       timeout { exit 1 }
       "401"
}
expect {
       timeout { exit 1 }
       "401"
}

send "kick ,, ,,,\r"
expect {
       timeout { exit 1 }
       "461"
}

send "kick ,, ,,\r"
expect {
       timeout { exit 1 }
       "401"
}
expect {
       timeout { exit 1 }
       "401"
}
expect {
       timeout { exit 1 }
       "401"
}

send "quit\r"
expect {
       timeout { exit 1 }
       "ERROR :Closing connection"
}
