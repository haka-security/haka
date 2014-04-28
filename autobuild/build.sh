#! /bin/bash

source autobuild/vars.sh
source autobuild/includes.sh

_build() {
	cd "$MAKEDIR/$1-$2"
	_run make all
}

_runeach _build

exit 0
