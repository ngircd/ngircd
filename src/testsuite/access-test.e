# ngIRCd test suite
# Channel access test (bans, invites, keys) using multiple clients

spawn telnet 127.0.0.1 6789
set op $spawn_id
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick op\r"
send "user user . . :Operator\r"
expect {
	timeout { exit 1 }
	"376"
}

spawn telnet 127.0.0.1 6789
set member $spawn_id
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick member\r"
send "user user . . :Member\r"
expect {
	timeout { exit 1 }
	"376"
}

spawn telnet 127.0.0.1 6789
set guest $spawn_id
expect {
	timeout { exit 1 }
	"Connected"
}
send "nick guest\\x\r"
send "user user . . :Guest\r"
expect {
	timeout { exit 1 }
	"376"
}

send -i $op "join #access\r"
expect {
	-i $op
	timeout { exit 1 }
	"366"
}
send -i $member "join #access\r"
expect {
	-i $member
	timeout { exit 1 }
	"366"
}

# A literal backslash following a "*" must match in masks
send -i $op "mode #access +b *\\x\r"
expect {
	-i $op
	timeout { exit 1 }
	"@* MODE #access +b "
}
send -i $guest "join #access\r"
expect {
	-i $guest
	timeout { exit 1 }
	"474"
}

# Regular members can invite, but not override bans
send -i $member "invite guest\\x #access\r"
expect {
	-i $member
	timeout { exit 1 }
	"341 member "
}
expect {
	-i $guest
	timeout { exit 1 }
	" INVITE guest"
}
send -i $guest "join #access\r"
expect {
	-i $guest
	timeout { exit 1 }
	"474"
}

# Channel operators can override bans
send -i $op "invite guest\\x #access\r"
expect {
	-i $op
	timeout { exit 1 }
	"341 op "
}
send -i $guest "join #access\r"
expect {
	-i $guest
	timeout { exit 1 }
	"@* JOIN :#access"
}

# A failed JOIN must not use up the invite
send -i $op "join #keyed\r"
expect {
	-i $op
	timeout { exit 1 }
	"366"
}
send -i $op "mode #keyed +ik key\r"
expect {
	-i $op
	timeout { exit 1 }
	"@* MODE #keyed +ik key"
}
send -i $op "invite guest\\x #keyed\r"
expect {
	-i $op
	timeout { exit 1 }
	"341 op "
}
send -i $guest "join #keyed\r"
expect {
	-i $guest
	timeout { exit 1 }
	"475"
}
send -i $guest "join #keyed key\r"
expect {
	-i $guest
	timeout { exit 1 }
	"@* JOIN :#keyed"
}

# Adding the mask of a single-use invite to the invite list makes it permanent
send -i $guest "part #keyed\r"
expect {
	-i $guest
	timeout { exit 1 }
	"@* PART #keyed"
}
send -i $op "invite guest\\x #keyed\r"
expect {
	-i $op
	timeout { exit 1 }
	"341 op "
}
send -i $op "mode #keyed I\r"
expect {
	-i $op
	timeout { exit 1 }
	-re "346 op #keyed (\[^ \]+) "
}
set mask $expect_out(1,string)
expect {
	-i $op
	timeout { exit 1 }
	"347 op #keyed"
}
send -i $op "mode #keyed +I $mask\r"
expect {
	-i $op
	timeout { exit 1 }
	"@* MODE #keyed +I "
}
foreach i {1 2} {
	send -i $guest "join #keyed key\r"
	expect {
		-i $guest
		timeout { exit 1 }
		"@* JOIN :#keyed"
	}
	send -i $guest "part #keyed\r"
	expect {
		-i $guest
		timeout { exit 1 }
		"@* PART #keyed"
	}
}

# Unregistered connections can't send NOTICE commands
spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}
send "notice op :unregistered\r"
send "privmsg op :unregistered\r"
expect {
	timeout { exit 1 }
	"451"
}
send -i $op "privmsg op :marker\r"
expect {
	-i $op
	timeout { exit 1 }
	"unregistered" { exit 1 }
	"@* PRIVMSG op :marker"
}
send "quit\r"
expect {
	timeout { exit 1 }
	"ERROR :Closing connection"
}

foreach id [list $guest $member $op] {
	send -i $id "quit\r"
	expect {
		-i $id
		timeout { exit 1 }
		"ERROR :Closing connection"
	}
}
