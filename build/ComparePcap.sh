#!/bin/bash

$TSHARK -r $1 -V | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $1).txt
$TSHARK -r $2 -V | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $2).txt

$DIFF $(basename $1).txt $(basename $2).txt > $(basename $1).diff
RET=$?

echo "original pcap file is $PWD/$1"
echo "generated pcap file is $2"
echo "diff file is $PWD/$(basename $1).diff"

cat $(basename $1).diff
exit $RET

