%define name    ngircd
%define version 0.7.0-pre2
%define release 1
%define prefix  %{_prefix}

Summary:      Next Generation Internet Relay Chat Daemon
Name:         %{name}
Version:      %{version}
Release:      %{release}
Copyright:    GPL
Group:        Networking/Daemons
URL:          http://arthur.ath.cx/~alex/ngircd/
Source:       %{name}-%{version}.tar.gz
Packager:     Sean Reifschneider <jafo-rpms@tummy.com>
BuildRoot:    /var/tmp/%{name}-root

%description
ngIRCd is a free open source daemon for Internet Relay Chat (IRC),
developed under the GNU General Public License (GPL). It's written from
scratch and is not based upon the original IRCd like many others.

Why should you use ngIRCd? Because ...

   * ... there are no problems with servers on changing or non-static IP
         addresses.
   * ... there is a small and lean configuration file.
   * ... there is a free, modern and open source C source code.
   * ... it is still under active development. 

%prep
%setup
%build
%configure
make

%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"
%makeinstall
(
   cd "$RPM_BUILD_ROOT"
   ( cd usr/sbin; mv *-ngircd ngircd )
   ( cd usr/share/man/man5; mv *-ngircd.conf.5 ngircd.conf.5 )
   ( cd usr/share/man/man8; mv *-ngircd.8 ngircd.8 )
)

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(755,root,root)
%doc AUTHORS  COPYING  ChangeLog  INSTALL NEWS  README
%config(noreplace) /etc
%{_prefix}/sbin
%{_prefix}/share/man/
