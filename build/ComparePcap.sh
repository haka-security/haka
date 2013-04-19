#!/bin/bash

tshark -r $1 -V | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $1)
tshark -r $2 -V | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $2)

diff $(basename $1) $(basename $2) > $(basename $1).diff
RET=$?

cat $(basename $1).diff
exit $RET

