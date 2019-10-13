#!/bin/bash
# Run this script from the buildlinux folder to build a redhat/rpm package
qmake DEFINES+="Q_OS_REDHAT" ../src
make
cp celovpn/celovpn linuxfiles
cp service/celovpnservice linuxfiles
cp netdown/netdown linuxfiles
cp openvpn64 linuxfiles/openvpn

# Then the content of linuxfiles mostly goes into /opt/celovpn/.

# To package redhat/rpm do the following:

tar --transform "s/^linuxfiles/celovpn-$1/" -zcpvf ~/rpmbuild/SOURCES/celovpn-$1.tar.gz linuxfiles
rpmbuild --define "debug_package %{nil}" -ba --sign -v ./celovpn.spec

