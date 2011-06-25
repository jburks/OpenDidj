#!/bin/bash

ZLIB_SRC=zlib-1.2.3.tar.bz2

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/zlib

if [ ! -e $ZLIB_SRC ]; then
	wget http://www.zlib.net/$ZLIB_SRC
fi

ZLIB_DIR=`echo "$ZLIB_SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $ZLIB_DIR ]; then
	rm -rf $ZLIB_DIR
	tar -xjf $ZLIB_SRC
fi

pushd $ZLIB_DIR

GCC=gcc
GLD=ld

CC=$CROSS_COMPILE$GCC LDSHARED="$CROSS_COMPILE$GLD -shared" ./configure --shared

# If this is the embedded build, we don't want headers or man pages
if [ $EMBEDDED -eq 1 ]; then
	patch -p1 < ../zlib-1.2.3-nomanpages-noincludes.patch
fi

make
make prefix=$ROOTFS_PATH/usr install

CC=$CROSS_COMPILE$GCC ./configure
make clean
make
make prefix=$ROOTFS_PATH/usr install
popd

popd

exit 0
