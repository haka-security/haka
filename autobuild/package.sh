#! /bin/bash

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
