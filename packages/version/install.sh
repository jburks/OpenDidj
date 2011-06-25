#!/bin/bash

CURRENT_MAJOR_VERSION=1.35.2

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

BUILD=`svn info $PROJECT_PATH | grep Revision | awk '{print $2}'`
VERSION=$CURRENT_MAJOR_VERSION-$BUILD
mkdir -p $ROOTFS_PATH/etc/
echo $VERSION > $ROOTFS_PATH/etc/version
exit 0

