# ngIRCd test suite
# Configuration reload test

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

send "oper TestOp 123\r"
expect {
	timeout { exit 1 }
	"381 nick"
}

send "mode nick +s\r"
expect {
	timeout { exit 1 }
	"MODE nick :+s"
}

# Pre-defined channels are created again when they have been deleted
foreach i [list 1 2] {
	send "mode #TopicChannel -P\r"
	expect {
		timeout { exit 1 }
		"@* MODE #TopicChannel -P\r"
	}
	send "mode #TopicChannel\r"
	expect {
		timeout { exit 1 }
		"403 nick #TopicChannel"
	}

	send "rehash\r"
	expect {
		timeout { exit 1 }
		"382 nick"
	}
	expect {
		timeout { exit 1 }
		"Re-reading of configuration done"
	}

	send "mode #TopicChannel\r"
	expect {
		timeout { exit 1 }
		"324 nick #TopicChannel +Pt"
	}
	send "topic #TopicChannel\r"
	expect {
		timeout { exit 1 }
		"332 nick #TopicChannel :the topic"
	}
}

# The random CloakHostSalt doesn't change when reloading
send "mode nick +x\r"
expect {
	timeout { exit 1 }
	-re "396 nick (\[^ \]+) "
}
set cloaked $expect_out(1,string)

send "rehash\r"
expect {
	timeout { exit 1 }
	"Re-reading of configuration done"
}

# Clients registered after the reload get the same cloaked hostname
set main $spawn_id
spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick cloaked\r"
send "user user . . :User\r"
expect {
	timeout { exit 1 }
	"376"
}
send "mode cloaked +x\r"
expect {
	timeout { exit 1 }
	-ex "396 cloaked $cloaked " {}
	"396 cloaked " { exit 1 }
}
send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
set spawn_id $main

# The configuration is kept when the file can't be read
exec mv T-ngircd1.conf T-ngircd1.conf.moved
send "rehash\r"
expect {
	timeout { exit 1 }
	"Re-reading configuration NOW!"
}
exec mv T-ngircd1.conf.moved T-ngircd1.conf

send "oper TestOp 123\r"
expect {
	timeout { exit 1 }
	"Re-reading of configuration done" { exit 1 }
	"381 nick"
}

send "mode #TopicChannel\r"
expect {
	timeout { exit 1 }
	"324 nick #TopicChannel +Pt"
}

set main $spawn_id
spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}
close
wait
set spawn_id $main

send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}
