#!/bin/awk
BEGIN {
	show = 0
	trace = 0
	alert = 0
}

$0 ~ /^[^ \t]+[ \t]+[^:]+:[ ]+.*$/ {
	/* Reformat log output to avoid extra spaces */
	$0 = $1 " " $2 " " substr($0, index($0,$3));
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

$0 ~ /^debug state-machine:/ {
	next;
}

$0 ~ /^debug event: signal/ {
	next;
}

$0 ~ /^stack traceback:$/ {
	trace = 1;
	next;
}

$0 ~ /^info alert: update id = / {
	alert = 1;
	print($1 " " $2 " " $3 " " $4 " = <>");
	next;
}

$0 ~ /^info alert:/ {
	alert = 1;
	print($1 " " $2 " " $3 " = <>");
	next;
}

$0 ~ /^\ttime = / {
	if (!alert) print;
	next;
}

$0 ~ /^[^ \t]/ {
	trace = 0;
	alert = 0;
}

{
	if (show == 2 && trace == 0) {
		print;
	}
	else if (show == 1) {
		show = 2
	}
}
