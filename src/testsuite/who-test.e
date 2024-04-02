# ngIRCd test suite
# WHO test

spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}

send "nick nick\r"
send "user user . . :Real Name\r"
expect {
	timeout { exit 1 }
	"376"
}

send "who\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick H :0 Real Name"
}

send "who 0\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick H :0 Real Name"
}

send "away :testing\r"
expect {
	timeout { exit 1 }
	"306 nick"
}

send "who *\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick G :0 Real Name"
}

send "join #channel\r"
expect {
	timeout { exit 1 }
	"@* JOIN :#channel"
}

send "who #channel\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick #channel * * ngircd.test.server nick G@ :0 Real Name"
}

send "mode #channel +v nick\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel +v nick\r"
}

send "who #channel\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick #channel * * ngircd.test.server nick G@ :0 Real Name"
}

send "who 127.0.0.*\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick G :0 Real Name"
}

send "mode #channel -o nick\r"
expect {
	timeout { exit 1 }
	"@* MODE #channel -o nick\r"
}

send "who #channel\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick #channel * * ngircd.test.server nick G+ :0 Real Name"
}

send "who ngircd.test.server\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick G :0 Real Name"
}

send "part #channel\r"
expect {
	timeout { exit 1 }
	"@* PART #channel :"
}

send "who Real?Name\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick G :0 Real Name"
}

send "oper TestOp 123\r"
expect {
	timeout { exit 1 }
	"MODE nick :+o"
}
expect {
	timeout { exit 1 }
	"381 nick"
}

send "who 0 o\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick G* :0 Real Name"
}

send "away\r"
expect {
	timeout { exit 1 }
	"305 nick"
}

send "who ??7.*0*\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick H* :0 Real Name"
}

send "join #opers\r"
expect {
	timeout { exit 1 }
	"@* JOIN :#opers"
}

send "who #opers\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick #opers * * ngircd.test.server nick H*@ :0 Real Name"
}

send "who Re*me\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick H* :0 Real Name"
}

send "mode #opers -o nick\r"
expect {
	timeout { exit 1 }
	"@* MODE #opers -o nick\r"
}

send "who #opers\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick #opers * * ngircd.test.server nick H* :0 Real Name"
}

send "who *.server\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * * ngircd.test.server nick H* :0 Real Name"
}

send "mode #opers +v nick\r"
expect {
	timeout { exit 1 }
	"@* MODE #opers +v nick\r"
}

send "who #opers\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick #opers * * ngircd.test.server nick H*+ :0 Real Name"
}

send "mode #opers +s\r"
expect {
	timeout { exit 1 }
	"@* MODE #opers +s\r"
}

send "who n?c?\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick \* * ngircd.test.server nick H* :0 Real Name"
}
expect {
	timeout { exit 1 }
	"315"
}

send "who #SecretChannel\r"
expect {
	timeout { exit 1 }
	"315"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
