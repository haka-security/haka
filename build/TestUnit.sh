#!/bin/bash

export LD_LIBRARY_PATH=$1
shift

$*
exit $?
