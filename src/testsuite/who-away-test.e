# $Id: who-away-test.e,v 1.1 2008/02/11 11:06:32 fw Exp $

spawn telnet localhost 6789
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
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick H :0 Real Name"
}

send "who 0\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick H :0 Real Name"
}

send "who *\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick H :0 Real Name"
}

send "away :testing\r"
expect {
	timeout { exit 1 }
	"306 nick"
}

send "who localhost\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick G :0 Real Name"
}

send "who ngircd.test.server\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick G :0 Real Name"
}

send "who Real?Name\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick G :0 Real Name"
}

send "who nick\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick G :0 Real Name"
}

send "away\r"
expect {
	timeout { exit 1 }
	"305 nick"
}

send "who *cal*ho??\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick H :0 Real Name"
}

send "who *.server\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick H :0 Real Name"
}

send "who Real*me\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick H :0 Real Name"
}

send "who n?c?\r"
expect {
	timeout { exit 1 }
	":ngircd.test.server 352 nick * ~user localhost ngircd.test.server nick H :0 Real Name"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}

# -eof-
