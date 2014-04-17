# ngIRCd test suite
# "Stress" header

set timeout 30

spawn telnet localhost 6789
expect {
	timeout { exit 1 }
	"Connected"
}
