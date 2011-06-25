#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/init

cp -fra etc/ $ROOTFS_PATH/
cp -fra usr/ $ROOTFS_PATH/
cp -fra var/ $ROOTFS_PATH/

pushd $ROOTFS_PATH
find . -name ".svn" | xargs rm -rf
popd

pushd $ROOTFS_PATH/etc
rm -f mtab
ln -s /proc/mounts mtab
popd

popd

exit 0
