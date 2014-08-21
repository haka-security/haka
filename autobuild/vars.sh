#! /bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

export PATH=/bin:/usr/bin:/usr/local/bin
export ROOT=$(pwd)
export MAKEDIR="$ROOT/workspace/"

echo $HAKA_LUA $HAKA_BUILD

if [ -z "$HAKA_BUILD" ]; then
	export HAKA_BUILD="Debug,Release"
fi

if [ -z "$HAKA_LUA" ]; then
	export HAKA_LUA="lua,luajit"
fi
