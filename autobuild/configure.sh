#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

source autobuild/vars.sh
source autobuild/includes.sh

### Init submodules
_run git submodule init
_run git submodule update

### Cleanup
_run rm -rf "$MAKEDIR"

_configure() {
	_run mkdir -p "$MAKEDIR/$1-$2"
	cd "$MAKEDIR/$1-$2"

	_run cmake -DBUILD=$1 -DLUA=$2 -DCMAKE_INSTALL_PREFIX=/usr -DJOBS=$JOBS "$ROOT"
}

_runeach _configure

### Success
exit 0
