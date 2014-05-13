#! /bin/bash

source autobuild/vars.sh
source autobuild/includes.sh

_doc() {
	cd "$MAKEDIR/$1-$2"
	_run make doc

	if [ -n "$DROPDIR" ]; then
		### Copy to destination
		_run mkdir -p "$DROPDIR/"
		_run cp -r doc/doc "$DROPDIR/"
	fi
}

_runeach _doc

exit 0
