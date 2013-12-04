# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# List of commands used to generate the pcap file

wget http://server

echo "WRONG / HTTP/1.1" > http
echo "" >> http
cat http | nc server 80

echo "GET / HTTP/2.1" > http
echo "" >> http
cat http | nc server 80

echo "TRACE / HTTP/1.1" > http
echo "" >> http
cat http | nc server 80

sqlmap -u http://server/
