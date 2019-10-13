#!/bin/bash
MAC_ID="Three Monkeys International Inc."

# sudo cp netdown/netdown Resources/
# install celo.icns "celovpn/Celo VPN.app/Contents/Resources/"
# sudo cp -R Resources/* "celovpn/Celo VPN.app/Contents/Resources/"
# /usr/libexec/PlistBuddy -c "Set :CFBundleIconFile celo.icns" "celovpn/Celo VPN.app/Contents/Info.plist"
# macdeployqt Celo VPN.app -verbose=1
# macdeployqt "celovpn/Celo VPN.app" -verbose=1 -codesign="Developer ID Application: $MAC_ID" 
# codesign -f -s "Developer ID Application: $MAC_ID" "celovpn/Celo VPN.app"
sh scripts/package.sh
hdiutil mount celovpn.dmg
scripts/ddstoregen.sh celovpn
hdiutil detach /Volumes/celovpn/

mv celovpn.dmg tmp.dmg
hdiutil convert -format UDRO -o celovpn.dmg tmp.dmg
rm tmp.dmg
hdiutil convert celovpn.dmg -format UDZO -imagekey -zlib-level=9 -o celovpn-compressed.dmg
rm celovpn.dmg
mv celovpn-compressed.dmg celovpn.dmg
