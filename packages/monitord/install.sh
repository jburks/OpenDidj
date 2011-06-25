#!/bin/bash

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/monitord

if [ "$CLEAN" == "1" ]; then
	make clean
fi

make
cp ./monitord $ROOTFS_PATH/usr/bin/
#cp ./test_monitord $ROOTFS_PATH/usr/bin/
#cp ./monitor-ctl $ROOTFS_PATH/usr/bin/
cp ./rl $ROOTFS_PATH/usr/bin/
popd

exit 0
