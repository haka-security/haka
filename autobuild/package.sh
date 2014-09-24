#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

source autobuild/vars.sh
source autobuild/includes.sh

_package() {
	cd "$MAKEDIR/$1-$2"

	### Build package
	_run make package
	_run make package_source

	if [ -n "$DROPDIR" ]; then
		### Copy to destination
		_run mkdir -p "$DROPDIR/"
		_run cp haka*.* "$DROPDIR/"
	fi
}

_runeach _package

exit 0
