#!/bin/bash

SRC=termcap-1.3.1.tar.gz
SRC_URL=ftp://ftp.gnu.org/pub/gnu/termcap/

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/termcap

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
fi

pushd $BUILD_DIR
GCC=gcc
GRANLIB=ranlib
CC=$CROSS_COMPILE$GCC RANLIB=$CROSS_COMPILE$GRANLIB ./configure --host=arm-linux --target=arm-linux --prefix=$ROOTFS_PATH/usr
make
popd

popd

exit 0
