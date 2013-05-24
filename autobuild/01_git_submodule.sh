#! /bin/bash

# This scripts clone the repository
# Internet access is needed to clone the luajit part

### VARS
source autobuild/vars.sh

### FUNCTIONS
source autobuild/includes.sh

_init_submodule(){
	echo "git submodule init"
	git submodule init
	if [ $? != 0 ]
	then
		echo "Error... Leaving autobuild scripts"
		exit 1
	fi
}


_update_submodule(){
	echo "git submodule update"
	git submodule update
	if [ $? != 0 ]
	then
		echo "Error... Leaving autobuild scripts"
		exit 1
	fi
}

### MAIN
_init_submodule
_update_submodule
exit 0
