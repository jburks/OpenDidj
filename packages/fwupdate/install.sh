#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/fwupdate

GCC=gcc

$CROSS_COMPILE$GCC $INCLUDES -o ver ver.c

cp -pv blcheck blcheck-cart blupdate \
      fwcheck fwcheck-cart fwupdate \
      pkupdate-cart ver $ROOTFS_PATH/usr/bin

popd

exit 0
