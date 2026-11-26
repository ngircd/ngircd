#!/bin/sh
# Builds every source tree under /src/<variant> into /opt/<variant>.
set -e
for d in /src/*/; do
  v=$(basename "$d")
  cp -r "$d" /build/$v && cd /build/$v
  ./autogen.sh --prefix=/opt/$v --with-openssl >/build/$v.cfg.log 2>&1 || { tail -30 /build/$v.cfg.log; exit 1; }
  make -j4 >/build/$v.make.log 2>&1 || { tail -30 /build/$v.make.log; exit 1; }
  make install >/dev/null
  /opt/$v/sbin/ngircd --version | head -1
done
mkdir -p /conf
openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -subj /CN=irc.example.net \
  -keyout /conf/key.pem -out /conf/cert.pem 2>/dev/null
