#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=dosfstools
SRC=dosfstools-2.11.src.tar.gz
SRC_URL=ftp://ftp.uni-erlangen.de/pub/Linux/LOCAL/dosfstools/

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/$PKG_NAME

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -2`

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
	pushd $BUILD_DIR
	patch -p1 < ../handle_unaligned_access_properly.patch
	popd
fi

pushd $BUILD_DIR
GCC=gcc
CC=$CROSS_COMPILE$GCC make -C mkdosfs/
make install SBINDIR=$ROOTFS_PATH/sbin MANDIR=/tmp/man -C mkdosfs/
CC=$CROSS_COMPILE$GCC make -C dosfsck/
make install SBINDIR=$ROOTFS_PATH/sbin MANDIR=/tmp/man -C dosfsck/
rm -rf /tmp/man
popd

popd

exit 0
