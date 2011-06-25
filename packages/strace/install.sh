#!/bin/bash

SRC=strace-4.5.15.tar.bz2
SRC_URL=http://downloads.sourceforge.net/strace

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/strace

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xjf $SRC
fi

pushd $BUILD_DIR

GCC=gcc
GLD=ld

MANDIR=/tmp/strace/
./configure --prefix=$ROOTFS_PATH/usr/ CC=$CROSS_COMPILE$GCC --host=arm-linux --mandir=$MANDIR
make
make install
rm -rf $MANDIR
popd

popd

exit 0
