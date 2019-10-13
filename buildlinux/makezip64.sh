#!/bin/bash
# Before executing this script make sure you have the 64 bit debian package installed.
cat files.txt | zip celovpn-linux-64.zip -@
zip -r celovpn-linux-64.zip /opt/celovpn 
