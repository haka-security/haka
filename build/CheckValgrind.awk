#!/bin/awk
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

BEGIN {
	error = 10000
	leak = 0
	reachable = 0
}

$0 ~ /==.*== ERROR SUMMARY: .* errors from .*/ {
	sub(",", "", $4)
	error = $4
}

$0 ~ /==.*==.*definitely lost: .* bytes in .* blocks/ {
	sub(",", "", $4)
	leak += $4
}

$0 ~ /==.*==.*indirectly lost: .* bytes in .* blocks/ {
	sub(",", "", $4)
	reachable += $4
}

$0 ~ /==.*==.*possibly lost: .* bytes in .* blocks/ {
	sub(",", "", $4)
	leak += $4
}

$0 ~ /==.*==.*still reachable: .* bytes in .* blocks/ {
	sub(",", "", $4)
	reachable += $4
}

END {
	printf "%d;%d;%d", error, leak, reachable
}
