#! /bin/bash

# Creating the make directory and configure things

### VARS
source autobuild/vars.sh

### FUNCTIONS
source autobuild/includes.sh

_create_dir(){
	if [ -d "$MAKDEIR" ]
	then
		echo "Directory $MAKEDIR is present although everything was cleaned: problem"
		exit 1
	fi
	mkdir "$MAKEDIR"
	if [ $? != 0 ]
	then
		echo "Unable to create $MAKEDIR"
		exit 1
	fi
}

_configure(){
	cd "$MAKEDIR"
	cmake ../
	if [ $? != 0 ]
	then
		echo "Problem during configure"
		exit 1
	fi
}

### MAIN
_create_dir
_configure
exit 0
