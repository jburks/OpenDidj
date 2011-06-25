#!/bin/bash
#
# Note: we grab the procps source and build just pmap from it (the rest of our
#       procps tools come from busybox, which doesn't have a pmap applet)

BUILD_FROM_SOURCE=1

SRC=procps-3.2.7.tar.gz
SRC_URL=http://procps.sourceforge.net
set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/pmap

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=procps-3.2.7

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
fi

pushd $BUILD_DIR

CC=arm-linux-gcc make pmap
cp ./pmap $ROOTFS_PATH/usr/bin
cp ./proc/libproc-3.2.7.so $ROOTFS_PATH/usr/lib

popd

popd

exit 0
