#! /bin/bash

# Compiling main project

### VARS
source autobuild/vars.sh

### FUNCTIONS
source autobuild/includes.sh

### MAIN
echo "Testing..."
cd "$MAKEDIR"
ctest --output-on-failure
if [ $? != 0 ]
then
	echo "Error... Exiting autobuild"
	exit 1
fi
exit 0
