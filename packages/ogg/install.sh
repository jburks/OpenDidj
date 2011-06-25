#!/bin/bash

# Build ogg libs from source
OGG_LIB_SRC=libogg-1.1.3.tar.gz

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/ogg/

if [ ! -e $OGG_LIB_SRC ]; then
	wget http://downloads.xiph.org/releases/ogg/$OGG_LIB_SRC
fi

OGG_LIB_DIR=`echo "$OGG_LIB_SRC" | cut -d '.' -f -3`
echo $OGG_LIB_DIR

if [ "$CLEAN" == "1" -o ! -e $OGG_LIB_DIR ]; then
	rm -rf $OGG_LIB_DIR
	tar -xzf $OGG_LIB_SRC
fi

# build and copy shared libs to rootfs
pushd $OGG_LIB_DIR
./configure --host=arm-linux --build=x86-linux --prefix=$ROOTFS_PATH/usr/local --enable-shared=yes
make
# make install
cp -a ./src/.libs/libogg.so* $ROOTFS_PATH/usr/local/lib/
cp -R ./include/ogg $ROOTFS_PATH/usr/local/include/
popd

popd

exit 0
