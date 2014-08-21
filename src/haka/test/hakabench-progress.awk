#!/bin/awk
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

$0 ~ /^info  benchmark: progress/ {
	printf " %5s %%\033[8D", $4
	fflush()
}
$0 ~ /^info  benchmark: processing/ {
	printf "Passed %13s Mib/s\033[K\n", $10
}
