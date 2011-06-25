#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=popt
SRC=popt-1.10.4.tar.gz
SRC_URL=http://rpm.net.in/mirror/rpm-4.4.x/

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

if [ "$BUILD_FROM_SOURCE" == "0" ]; then
	pushd $PROJECT_PATH/packages/$PKG_NAME/
	svn export binary /tmp/$PKG_NAME-binary/
	cp -ra /tmp/$PKG_NAME-binary/* $ROOTFS_PATH/
	rm -rf /tmp/$PKG_NAME-binary/
	popd
	exit 0
fi

pushd $PROJECT_PATH/packages/$PKG_NAME

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
fi

pushd $BUILD_DIR
./configure --host=arm-linux --prefix=/usr
make
make install DESTDIR=$ROOTFS_PATH
popd

popd

exit 0
