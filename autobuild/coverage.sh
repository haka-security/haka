#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

source autobuild/vars.sh
source autobuild/includes.sh

_coverage() {
	_run mkdir -p "$MAKEDIR/Coverage-$1"
	cd "$MAKEDIR/Coverage-$1"

	_run cmake -DBUILD=Coverage -DLUA=$1 "$ROOT"
	_run make all
	_run make coverage_clean
	VALGRIND=0 make tests
	_run make coverage

	if [ -n "$DROPDIR" ]; then
		_run mkdir -p "$DROPDIR/coverage-$1"
		_run cp -r coverage "$DROPDIR/coverage-$1"
	fi
}

_runeachoptions _coverage

exit 0
