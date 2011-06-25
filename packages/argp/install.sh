#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=argp
SRC=argp-standalone-1.3.tar.gz
SRC_URL=http://www.lysator.liu.se/~nisse/misc/
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
fi

pushd $BUILD_DIR

# eliminate unsupported __THROW macro
sed 's/__THROW//' < argp-parse.c > tmpfile
mv tmpfile argp-parse.c

./configure --host=arm-linux --target=arm-linux --prefix=/usr --enable-shared
make
make install DESTDIR=$ROOTFS_PATH

popd

popd

exit 0
