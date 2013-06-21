#!/bin/awk
BEGIN {
	show = 0
}

$0 ~ /info core: starting single threaded processing/ {
	show = 1;
	next;
}

$0 ~ /info core: unload module/ {
	show = 0;
	next;
}

$0 ~ /^debug packet:/ {
	next;
}

$0 ~ /^debug rule:/ {
	next;
}

{
	if (show == 2) {
		print;
	}
	else if (show == 1) {
		show = 2
	}
}

