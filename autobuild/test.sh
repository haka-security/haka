#! /bin/bash

source autobuild/vars.sh
source autobuild/includes.sh

_tests() {
	cd "$MAKEDIR/$1-$2"

	_run make tests CTEST_ARGS="--output-on-failure"
}

_runeach _tests

exit 0
