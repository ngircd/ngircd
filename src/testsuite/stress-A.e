# ngIRCd test suite
# "Stress" header

set timeout 30

spawn telnet 127.0.0.1 6789
expect {
	timeout { exit 1 }
	"Connected"
}
