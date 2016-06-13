#!/bin/awk
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

BEGIN {
	show = 0
	trace = 0
	alert = 0
	closing = 0
}

$0 ~ /^[^ \t]+[ \t]+[^:]+:[ ]+.*$/ {
	/* Reformat log output to avoid extra spaces */
	$0 = $1 " " $2 " " substr($0, index($0,$3));
}

$0 ~ /^[^ \t]/ {
	trace = 0;
	alert = 0;
}

$0 ~ /^debug packet:/ { next; }
$0 ~ /^debug pcre:/ { next; }
$0 ~ /^debug states:/ { next; }
$0 ~ /^debug event: signal/ { next; }
$0 ~ /^debug core: rejected policy .* next dissector$/ { next; }
$0 ~ /^debug core: applying( anonymous)? policy .* next dissector$/ { next; }
$0 ~ /^debug time: / { next; }
$0 ~ /^info capture: progress/ { next; }
$0 ~ /^debug core: memory report/ { next; }
$0 ~ /^warn core: rule .*at .* uses 'hook' keyword which is deprecated and should be replaced by 'on'$/ { next; }

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

$0 ~ /^debug grammar: in rule / {
	trace = 1;
	next;
}

$0 ~ /^stack traceback:$/ {
	trace = 1;
	next;
}

$0 ~ /^alert: update id = / {
	alert = 1;
	print($1 " " $2 " " $3 " " $4 " = <>");
	next;
}

$0 ~ /^alert:/ {
	alert = 1;
	print($1 " " $2 " " $3 " = <>");
	next;
}

$0 ~ /^\ttime = / {
	if (!alert) print;
	next;
}

$0 ~ /^debug lua: closing state$/ {
	closing = 1;
}

$0 ~ /^debug conn: .* connection/ {
	if (closing) {
		print($1 " " $2 " <cleanup> " $4);
		next;
	}
}

{
	if (show == 2 && trace == 0) {
		print;
	}
	else if (show == 1) {
		show = 2
	}
}
