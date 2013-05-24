#! /bin/bash
#

_make(){
	echo "Making $1"
	cd "$MAKEDIR"
	make $1
	if [ $? != 0 ]
	then
		echo "Error... Exiting autobuild"
		exit 1
	fi
}

