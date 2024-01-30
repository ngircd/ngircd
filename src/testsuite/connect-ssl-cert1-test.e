# ngIRCd test suite
# Server connect test

spawn openssl s_client -quiet -nameopt utf8,space_eq -connect 127.0.0.1:6790
expect {
        timeout { exit 1 }
        "*CN = my.first.domain.tld"
}

sleep 2
send "oper\r"
expect {
	timeout { exit 1 }
	"451"
}

send "quit\r"
expect {
	timeout { exit 1 }
	"Connection closed"
}
