#!/bin/bash
# Before executing this script make sure you have the 32 bit debian package installed.
cat files.txt | zip celovpn-linux-32.zip -@
zip -r celovpn-linux-32.zip /opt/celovpn 
