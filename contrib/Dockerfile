# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors

# Build Container

FROM docker.io/library/debian:stable-slim AS build
LABEL org.opencontainers.image.source=https://github.com/ngircd/ngircd
LABEL org.opencontainers.image.description="Free, portable and lightweight Internet Relay Chat server (build container)"
LABEL org.opencontainers.image.licenses=GPL-2.0-or-later
USER root
RUN apt-get -y update \
	&& apt-get -y install --no-install-recommends \
		autoconf \
		automake \
		build-essential \
		expect \
		gawk \
		git \
		libgnutls28-dev \
		libident-dev \
		libpam0g-dev \
		openssl \
		pkg-config \
		telnet \
		zlib1g-dev \
	&& mkdir -p /usr/local/src/ngircd /opt/ngircd \
	&& chown bin:bin /usr/local/src/ngircd /opt/ngircd
WORKDIR /usr/local/src/ngircd
COPY . /usr/local/src/ngircd
RUN chown -R bin /usr/local/src/ngircd
USER bin
RUN ./autogen.sh --prefix=/opt/ngircd \
		--with-gnutls \
		--with-iconv \
		--with-ident \
		--with-pam \
	&& make all \
	&& make -C src/ngircd check \
	&& make install \
	&& printf \
		"# ngircd.conf\n\n[Global]\nServerGID=irc\nServerUID=irc\n\n[Options]\nIdent=no\nPAM=no\n\n[SSL]\nCAFile=/etc/ssl/certs/ca-certificates.crt\n" \
		>/opt/ngircd/etc/ngircd.conf \
	&& chmod -R a+rX /opt/ngircd

# Run container

FROM docker.io/library/debian:stable-slim
LABEL org.opencontainers.image.source=https://github.com/ngircd/ngircd
LABEL org.opencontainers.image.description="Free, portable and lightweight Internet Relay Chat server"
LABEL org.opencontainers.image.licenses=GPL-2.0-or-later
USER root
RUN apt-get -y update \
	&& apt-get -y install --no-install-recommends --no-install-suggests \
		ca-certificates \
		catatonit \
		libgnutls30 \
		libident \
		libpam0g \
		libwrap0 \
		zlib1g \
	&& apt-get -y clean \
	&& rm -rf /var/cache/debconf/*-old /var/lib/apt/lists/*
COPY --from=build /opt/ngircd /opt/ngircd
USER irc
ENTRYPOINT [ "/usr/bin/catatonit", "--", "/opt/ngircd/sbin/ngircd", "--nodaemon" ]
EXPOSE 6667 6697
HEALTHCHECK --interval=30s --timeout=5s --retries=1 --start-period=5s \
	CMD [ "/usr/bin/grep", "-F", ":1A0B ", "/proc/net/tcp" ]
