#!/bin/awk
BEGIN {
	error = 10000
	leak = 10000
	reachable = 10000
}

$0 ~ /==.*== ERROR SUMMARY: .* errors from .*/ {
	sub(",", "", $4)
	error = $4
}

$0 ~ /==.*==.*definitely lost: .* bytes in .* blocks/ {
	sub(",", "", $4)
	leak = $4
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
	reachable = $4
}

END {
	printf "%d;%d;%d", error, leak, reachable
}
