#!/bin/bash

cat > /tmp/gnuplot.script <<EOF
	set term png small size 1200,800
	set output "/tmp/mem-graph.png"
	set ylabel "KB"
	set ytics nomirror
	set yrange [0:*]
	set y2label "KB"
	set y2tics nomirror in
	set y2range [0:*]

	plot "/tmp/mem.log" using 3 with lines axes x1y2 title "LUA", \
	     "/tmp/mem.log" using 1 with lines axes x1y1 title "VMSize", \
	     "/tmp/mem.log" using 2 with lines axes x1y1 title "RSSSize"
EOF

echo "0 0 0" > /tmp/mem.log
echo > /tmp/out.log

$* > /tmp/out.log &

gnuplot /tmp/gnuplot.script
eog /tmp/mem-graph.png &

while true; do
	echo "0 0 0" > /tmp/mem.log
	sed -n -e 's/^.*memory report:.*vmsize=\(.*\) rsssize=\(.*\) luasize=\(.*\)/\1 \2 \3/p' /tmp/out.log >> /tmp/mem.log
	gnuplot /tmp/gnuplot.script

	pidof $1 > /dev/null
	if [ $? != 0 ]; then
		break
	fi

	sleep 2
done
