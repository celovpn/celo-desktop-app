#!/bin/sh

VERSION=$1

scripts/pkg-dmg \
    --verbosity 2 \
    --volname "celovpn" \
    --source "celovpn/Celo VPN.app" \
    --sourcefile \
    --format UDRW \
    --target celovpn.dmg \
    --icon celo.icns  \
    --mkdir .background \
    --symlink  /Applications:Applications \

