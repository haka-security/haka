#! /bin/bash

export PATH=/bin:/usr/bin:/usr/local/bin
export ROOT=$(pwd)
export MAKEDIR="$ROOT/make/"

echo $HAKA_LUA $HAKA_BUILD

if [ -z "$HAKA_BUILD" ]; then
	export HAKA_BUILD="Debug,Release"
fi

if [ -z "$HAKA_LUA" ]; then
	export HAKA_LUA="lua,luajit"
fi
