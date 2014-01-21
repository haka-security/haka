# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# List of commands used to generate the pcap file

nmap -sX server

wget server
wget 192.168.20.255

ssh server
ssh 192.168.20.255

echo "GET / HTTP/1.1" > tcp
echo "" >> tcp
printf "\xeb\x1f\x5e\x89\x76\x08\x31\xc0\x88\x46\x07\x89\x46\x0c\xb0\x0b" >> tcp
printf "\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xcd\x80\x31\xdb\x89\xd8\x40\xcd" >> tcp
printf "\x80\xe8\xdc\xff\xff\xff/bin/sh" >> tcp
cat tcp | nc server 80
