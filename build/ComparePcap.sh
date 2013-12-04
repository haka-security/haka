#!/bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

TSHARK_ARGS="-o ip.check_checksum:TRUE -o udp.check_checksum:TRUE -o tcp.check_checksum:TRUE"

$TSHARK $TSHARK_ARGS -Vr $1 | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $1).txt
$TSHARK $TSHARK_ARGS -Vr $2 | gawk -f $BUILD_DIR/ComparePcap.awk - > $(basename $2).txt

$DIFF $(basename $1).txt $(basename $2).txt > $(basename $1).diff
RET=$?

cat $(basename $1).diff
exit $RET

