#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=libpng
SRC=libpng-1.2.22.tar.bz2
SRC_URL=http://superb-east.dl.sourceforge.net/sourceforge/libpng/
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

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xjf $SRC
fi

pushd $BUILD_DIR

CFLAGS="-I$PROJECT_PATH/packages/zlib/zlib-1.2.3/ -DPNG_NO_FLOATING_POINT_SUPPORTED -O3 -fPIC -mcpu=arm926ej-s" LDFLAGS=-L$PROJECT_PATH/packages/zlib/zlib-1.2.3/ ./configure --host=arm-linux --target=arm-linux --prefix=$ROOTFS_PATH/usr --enable-shared
make clean
make
make install

popd

popd

exit 0
