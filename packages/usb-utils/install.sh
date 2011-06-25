#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/usb-utils

cp -fra usr/bin/* $ROOTFS_PATH/usr/bin
pushd $ROOTFS_PATH
find . -name ".svn" | xargs rm -rf
popd

popd

exit 0
