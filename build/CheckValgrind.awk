#!/bin/awk
BEGIN {
	leak = 0
	reachable = 0
}

$0 ~ /==.*==.*definitely lost: .* bytes in .* blocks/ {
	sub(",", "", $4)
	leak += $4
}

$0 ~ /==.*==.*indirectly lost: .* bytes in .* blocks/ {
	sub(",", "", $4)
	leak += $4
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
	printf "%d;%d", leak, reachable
}
