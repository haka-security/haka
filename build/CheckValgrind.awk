#!/bin/awk
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

BEGIN {
	leak = 0
	reachable = 0
}

$0 ~ /==.*==.*definitely lost: .* bytes in .* blocks/ {
	leak += $4
}

$0 ~ /==.*==.*indirectly lost: .* bytes in .* blocks/ {
	leak += $4
}

$0 ~ /==.*==.*possibly lost: .* bytes in .* blocks/ {
	leak += $4
}

$0 ~ /==.*==.*still reachable: .* bytes in .* blocks/ {
	reachable += $4
}

END {
	printf "%d;%d", leak, reachable
}
