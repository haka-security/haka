#! /bin/bash
#

_run() {
	echo $*
	$*
	if [ $? != 0 ]
	then
		echo "Error"
		exit 1
	fi
}

_runeach() {
	for build in $(echo "$HAKA_BUILD" | sed s/,/\\n/g); do
		for lua in $(echo "$HAKA_LUA" | sed s/,/\\n/g); do
			$* $build $lua
		done
	done
}

