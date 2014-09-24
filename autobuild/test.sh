#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

source autobuild/vars.sh
source autobuild/includes.sh

_tests() {
	cd "$MAKEDIR/$1-$2"

	_run make tests CTEST_ARGS="--output-on-failure"
}

_runeach _tests

exit 0
