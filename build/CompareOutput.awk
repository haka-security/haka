#!/bin/awk
BEGIN {
	show = 0
}

$0 ~ /info: core: starting single threaded processing/ {
	show = 1;
	next;
}

$0 ~ /info: core: unload module/ {
	show = 0;
	next;
}

{
	if (show != 0) {
		print;
	}
}

