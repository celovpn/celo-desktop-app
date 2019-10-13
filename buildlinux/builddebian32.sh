#!/bin/bash
# Run this script from the buildlinux folder to build a debian/ubuntu package
qmake ../src
make clean
make
cp celovpn/celovpn linuxfiles
cp service/celovpnservice linuxfiles
cp netdown/netdown linuxfiles
cp openvpn32 linuxfiles/openvpn

# Then the content of linuxfiles mostly goes into /opt/celovpn/.

# To package debian/ubuntu do the following:

tar -zcpvf ../celovpn_$1_orig.tar.gz linuxfiles
cd ../
tar -zxpvf celovpn_$1_orig.tar.gz
cp -r buildlinux/debian linuxfiles/
cd linuxfiles
dpkg-buildpackage -b -uc -us -ai386

