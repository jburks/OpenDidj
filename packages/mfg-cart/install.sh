#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/mfg-cart

cp ./mkbase.sh $ROOTFS_PATH/usr/bin/
cp ./mfg-start.sh $ROOTFS_PATH/usr/bin/

popd

exit 0
