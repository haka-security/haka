#!/bin/awk
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

{
	print;
}

