#!/bin/sh
# Before/after test for the ngIRCd "cleartext after another client's TLS reset" bug.
#
#   ./run.sh [-r RUNS] [-l "libressl openssl"] [-C ngircd-repo] UNPATCHED_REF PATCHED_REF
#
# Defaults: RUNS=5, both TLS libraries, the ngIRCd repository this script is in.
# Extra args for repro.py can be passed via REPRO_ARGS="...".
set -e
cd "$(dirname "$0")"
HERE=$(pwd)
RUNS=5 LIBS="libressl openssl" REPO=${NGIRCD_REPO:-$(git rev-parse --show-toplevel)}
while getopts r:l:C: o; do
  case $o in r) RUNS=$OPTARG;; l) LIBS=$OPTARG;; C) REPO=$OPTARG;; *) exit 2;; esac
done
shift $((OPTIND - 1))
[ $# -eq 2 ] || { echo "usage: $0 [-r RUNS] [-l LIBS] [-C REPO] UNPATCHED_REF PATCHED_REF" >&2; exit 2; }
UNPATCHED=$1 PATCHED=$2

W=$HERE/work; rm -rf "$W/src"; mkdir -p "$W/src/unpatched" "$W/src/patched" "$W/logs"
for v in unpatched patched; do
  [ $v = unpatched ] && ref=$UNPATCHED || ref=$PATCHED
  echo "$v = $ref ($(git -C "$REPO" rev-parse --short "$ref"))"
  git -C "$REPO" archive "$ref" | tar -x -C "$W/src/$v"
done
cp -r docker ngircd.conf "$W/"
for lib in $LIBS; do
  echo "building ngrepro-standalone:$lib ..."
  docker build -q -f "$W/docker/Dockerfile.$lib" -t ngrepro-standalone:$lib "$W" >/dev/null
done

# one test: lib variant run-label A-close-mode [extra repro.py args]
one() {
  lib=$1 var=$2 n=$3 mode=$4; shift 4
  log=$W/logs/$lib-$var-$n.log
  docker run --rm --sysctl net.ipv4.tcp_wmem="4096 16384 16384" \
    -v "$HERE/repro.py:/repro.py:ro" ngrepro-standalone:$lib sh -c "
    /opt/$var/sbin/ngircd -n -f /conf/ngircd.conf >/tmp/ngircd.log 2>&1 &
    sleep 1; timeout 120 python3 -u /repro.py $REPRO_ARGS $*; sleep 1
    echo '### ngircd log'; cat /tmp/ngircd.log" >"$log" 2>&1 || true
  v=$(sed -n 's/^VERDICT //p' "$log")
  orphan=$(grep -c 'SSL error: .*\[in ConnSSL_Write()\]' "$log" || true)
  hdr=$(sed -n "s/.*framing breaks at raw offset [0-9]*: header bytes \([0-9a-f]*\).*/\1/p" "$log")
  bport=$(sed -n 's/.*local port \([0-9]*\)).*/\1/p' "$log")
  bclose=$(grep "Connection .* with \"127.0.0.1:$bport\" closed\|Shutting down connection .* (.*) with \"127.0.0.1:$bport\"" "$log" | sed -n 's/.*Shutting down connection [0-9]* (\(.*\)) with.*/\1/p' | head -1)
  printf "%-9s %-10s %-6s %-5s %-7s %-6s %-12s %s\n" "$lib" "$var" "$mode" "$n" "${v:-?}" "$orphan" "${hdr:--}" "${bclose:-open}"
}

printf "\n%-9s %-10s %-6s %-5s %-7s %-6s %-12s %s\n" LIB BUILD A RUN VERDICT ORPHAN BAD_HDR B_CLOSE_REASON
for lib in $LIBS; do
  for var in unpatched patched; do
    n=1; while [ $n -le $RUNS ]; do one $lib $var $n reset; n=$((n + 1)); done
  done
  one $lib unpatched ctl clean --no-reset
done
echo
echo "VERDICT: LEAK = canary text in cleartext on B's TLS socket; KILLED = B closed;"
echo "         OK = B got everything over TLS. ORPHAN = count of 'SSL error: ... [in ConnSSL_Write()]' log lines."
echo "B_CLOSE_REASON 'Client closed connection' = the test closing B at the end; BAD_HDR = first non-TLS 'record header' B saw."
echo "Logs: $W/logs/"
