%define name    ngircd
%define version 25
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
This package provides ngIRCd, a portable and lightweight Internet Relay
Chat server for small or private networks, developed under the GNU
General Public License (GPL). It is simple to configure, can cope with
dynamic IP addresses, and supports IPv6 as well as SSL. It is written
from scratch and not based on the original IRCd.

Advantages:
 - well arranged (lean) configuration file
 - simple to build/install, configure and maintain
 - supports IPv6 and SSL
 - no problems with servers that have dynamic IP addresses
 - freely available, modern, portable and tidy C-source
 - ngIRCd is being actively developed since 2001

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
%doc AUTHORS  COPYING  ChangeLog  INSTALL NEWS  README doc/*
%config(noreplace) /etc
%{_prefix}/sbin
%{_mandir}/man5/ngircd.conf*
%{_mandir}/man8/ngircd.8*
