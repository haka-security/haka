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
		_run cd doc/doc && tar -cz -f "$DROPDIR/haka-doc.tgz" *
	fi
}

_runeach _doc

exit 0
