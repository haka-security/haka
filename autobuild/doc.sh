#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
