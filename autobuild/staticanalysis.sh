#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

source autobuild/vars.sh
source autobuild/includes.sh

_staticanalysis() {
	_run mkdir -p "$MAKEDIR/StaticAnalysis-$1"
	cd "$MAKEDIR/StaticAnalysis-$1"

	rm -rf static_analysis
	mkdir -p static_analysis

	_run scan-build -o static_analysis cmake -DBUILD=Debug -DLUA=$1 "$ROOT"
	_run scan-build -o static_analysis make clean all

	if [ -n "$DROPDIR" ]; then
		_run mkdir -p "$DROPDIR/static_analysis-$1"
		_run cp -r static_analysis "$DROPDIR/static_analysis-$1"
	fi
}

_runeachoptions _staticanalysis

exit 0
