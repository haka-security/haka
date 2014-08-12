#! /bin/bash

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
