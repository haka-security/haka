#!/bin/awk
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

BEGIN {
}

$0 ~ /.*on interface .*$/ {
	sub(/ on interface .*$/, "", $0);
	print $0;
	next;
}

$0 ~ /Time/ {
	next;
}

$0 ~ /Interface id: .*$/ {
	next;
}

$0 ~ /The RTT to ACK the segment was: / {
	next;
}

{
	print;
}

