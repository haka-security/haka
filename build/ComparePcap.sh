#!/bin/bash

$TSHARK -r $1 -V | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $1).txt
$TSHARK -r $2 -V | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $2).txt

$DIFF $(basename $1).txt $(basename $2).txt > $(basename $1).diff
RET=$?

cat $(basename $1).diff
exit $RET

