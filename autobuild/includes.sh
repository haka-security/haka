#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

_runeachoptions() {
	for lua in $(echo "$HAKA_LUA" | sed s/,/\\n/g); do
		$* $lua
	done
}
