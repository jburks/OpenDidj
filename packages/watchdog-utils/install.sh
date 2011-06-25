#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/watchdog-utils

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make KERNELDIR=$KERNELDIR install

popd

exit 0
