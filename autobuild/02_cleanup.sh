#! /bin/bash

# This script cleans everything from previous builds

### VARS
source autobuild/vars.sh

### FUNCTIONS
source autobuild/includes.sh

_cleanup(){
	#We start from a clean clone: nothing to do
	true
}

### MAIN

_cleanup
exit 0
