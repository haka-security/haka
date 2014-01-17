#! /bin/bash

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

	_run cmake -DBUILD=$1 -DLUA=$2 "$ROOT"
}

_runeach _configure

### Success
exit 0
