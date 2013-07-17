#!/bin/awk
BEGIN {
	show = 0
	trace = 0
}

$0 ~ /warn core:/ {
	print;
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

$0 ~ /^stack traceback:$/ {
	trace = 1;
	next;
}

$0 ~ /^[^ \t]/ {
	trace = 0;
}

{
	if (show == 2 && trace == 0) {
		print;
	}
	else if (show == 1) {
		show = 2
	}
}
