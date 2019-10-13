#
# spec file for package celovpn
#
# Copyright (c) 2016 Jeremy Whiting <jeremypwhiting@gmail.com>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

Name:           celovpn
Summary:        VPN client for celo.net.
License:        GPL-2.0 and GPL-3.0
Group:          Productivity/Networking/Web/Utilities
Version:        2018.08.14
Release:        0
Url:            http://www.celo.net
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Source:         celovpn-%version.tar.gz
Conflicts:      celovpn < %version-%release
Requires:	net-tools
Requires:	redhat-rpm-config

# Do not check any files in env for requires
# %global __requires_exclude_from ^/opt/celovpn/env/.*$
# %global __requires_exclude ^/opt/celovpn/env/.*$

%description
VPN client for celo.net
celoVPN is a lightweight OpenVPN client specifically designed for the celo VPN network.

Authors:
--------
    Jeremy Whiting <jeremypwhiting@gmail.com>
#--------------------------------------------------------------------------------
%prep
%setup -q 

%build

%install
#
# First install all dist files
#
mkdir -p $RPM_BUILD_ROOT/opt/celovpn/
mkdir -p $RPM_BUILD_ROOT/usr/share/applications/
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/16x16/apps
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/192x192/apps
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/32x32/apps
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/512x512/apps
mkdir -p $RPM_BUILD_ROOT/usr/lib/systemd/system/
install -m 0755 celovpn                $RPM_BUILD_ROOT/opt/celovpn/celovpn
install -m 0755 celovpnservice         $RPM_BUILD_ROOT/opt/celovpn/celovpnservice
install -m 0755 netdown                $RPM_BUILD_ROOT/opt/celovpn/netdown
install -m 0755 openvpn                $RPM_BUILD_ROOT/opt/celovpn/openvpn
install -m 0744 client.down.celovpn.sh $RPM_BUILD_ROOT/opt/celovpn/client.down.celovpn.sh
install -m 0744 client.up.celovpn.sh   $RPM_BUILD_ROOT/opt/celovpn/client.up.celovpn.sh
# install -d 0644 env                       $RPM_BUILD_ROOT/opt/celovpn/env
install -m 0755 celovpn.desktop        $RPM_BUILD_ROOT/usr/share/applications
install -m 0644 celovpn.service        $RPM_BUILD_ROOT/usr/lib/systemd/system/celovpn.service
install -m 0744 icons/16x16/apps/celovpn.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/16x16/apps
install -m 0744 icons/192x192/apps/celovpn.png   $RPM_BUILD_ROOT/usr/share/icons/hicolor/192x192/apps
install -m 0744 icons/32x32/apps/celovpn.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/32x32/apps
install -m 0744 icons/512x512/apps/celovpn.png   $RPM_BUILD_ROOT/usr/share/icons/hicolor/512x512/apps

%pre

%preun
systemctl disable celovpn
systemctl stop celovpn

%post
systemctl enable celovpn
systemctl start celovpn

%posttrans

%postun

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc {README,LICENSE}
%dir /opt/celovpn/
/opt/celovpn/celovpn
/opt/celovpn/celovpnservice
/opt/celovpn/netdown
/opt/celovpn/openvpn
/opt/celovpn/client.down.celovpn.sh
/opt/celovpn/client.up.celovpn.sh
# /opt/celovpn/env
/usr/share/applications/celovpn.desktop
/usr/share/icons/hicolor/16x16/apps/celovpn.png
/usr/share/icons/hicolor/192x192/apps/celovpn.png
/usr/share/icons/hicolor/32x32/apps/celovpn.png
/usr/share/icons/hicolor/512x512/apps/celovpn.png
/usr/lib/systemd/system/celovpn.service

%changelog
