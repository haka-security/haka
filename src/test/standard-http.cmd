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
