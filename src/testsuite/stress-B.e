# ngIRCd test suite
# "Stress" body

send "user user . . :User\r"
expect {
	timeout { exit 1 }
	" 376"
}

sleep 2

send "oper TestOp 123\r"
expect {
	timeout { exit 1 }
	"MODE test* :+o"
}
expect {
	timeout { exit 1 }
	" 381 test"
}

sleep 2

send "join #channel\r"
expect {
	timeout { exit 1 }
	" 353 * = #channel "
}
expect {
	timeout { exit 1 }
	" 366 * #channel :"
}

send "mode #channel\r"
expect {
	timeout { exit 1 }
	" 324 test* #channel"
}

send "join #channel2\r"
expect {
	timeout { exit 1 }
	" 353 * = #channel2 "
}
expect {
	timeout { exit 1 }
	" 366 * #channel2 :"
}

send "names\r"
expect {
	timeout { exit 1 }
	" 366 "
}

sleep 3

send "part #channel2\r"
expect {
	timeout { exit 1 }
	" PART #channel2 "
}

send "part #channel\r"
expect {
	timeout { exit 1 }
	" PART #channel "
}

sleep 1

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
