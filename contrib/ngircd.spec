%define name    ngircd
%define version 27
%define release 1
%define prefix  %{_prefix}

Summary:      A lightweight daemon for the Internet Relay Chat (IRC)
Name:         %{name}
Version:      %{version}
Release:      %{release}
License:      GPLv2+
Group:        System Environment/Daemons
URL:          http://ngircd.barton.de/
Source:       %{name}-%{version}.tar.gz
BuildRoot:    %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  zlib-devel, openssl-devel

%description
ngIRCd is a free, portable and lightweight Internet Relay Chat server for small
or private networks, developed under the GNU General Public License (GPL).

The server is quite easy to configure, can handle dynamic IP addresses, and
optionally supports IDENT, IPv6 connections, SSL-protected links, and PAM for
user authentication as well as character set conversion for legacy clients. The
server has been written from scratch and is not based on the forefather, the
daemon of IRCNet.


%prep
%setup -q
%build
%configure \
  --with-zlib \
  --with-openssl

make %{?_smp_mflags}

%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"
%makeinstall
(
   cd "$RPM_BUILD_ROOT"
   ( cd usr/sbin; mv *-ngircd ngircd )
   ( cd usr/share/man/man5; mv *-ngircd.conf.5 ngircd.conf.5 )
   ( cd usr/share/man/man8; mv *-ngircd.8 ngircd.8 )
   rm -fr usr/share/doc/ngircd
)

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(755,root,root)
%doc AUTHORS.md COPYING ChangeLog INSTALL.md NEWS README.md doc/*
%config(noreplace) /etc
%{_prefix}/sbin
%{_mandir}/man5/ngircd.conf*
%{_mandir}/man8/ngircd.8*
