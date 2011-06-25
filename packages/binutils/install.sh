#!/bin/bash

BUILD_FROM_SOURCE=1

PKG_NAME=binutils
SRC=binutils-2.17.tar.bz2
SRC_URL=http://ftp.gnu.org/gnu/binutils/

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

SRC_DIR=`echo "$SRC" | cut -d '.' -f -2`
BUILD_DIR=binutils-build

if [ ! -e $SRC_DIR ]; then
	tar -xjf $SRC
fi

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	mkdir $BUILD_DIR
fi

pushd $BUILD_DIR

CROSS_COMPILE=arm-linux- ../$SRC_DIR/configure --host=arm-linux --target=arm-linux --prefix=/usr --build=i386-linux --enable-shared=yes
make
make install DESTDIR=$ROOTFS_PATH

# For mpatrol to work, we need libiberty.so, which is not supported!
pushd libiberty/pic/
${CROSS_COMPILE}gcc -shared --whole-archive -o libiberty.so *.o
cp libiberty.so $ROOTFS_PATH/usr/lib
popd

# install libintl for use with oprofile
cp intl/libintl.a $ROOTFS_PATH/usr/lib/

popd

popd

exit 0
