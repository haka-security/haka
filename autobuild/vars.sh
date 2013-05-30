#! /bin/bash

export MAKEDIR=make

export PATH=/bin:/usr/bin:/usr/local/bin

## Configure ARGS
if [ x"$USE_LUAJIT" == x"no" ]
then
	export CMAKE_ARGS="-DUSE_LUAJIT=no"
fi
