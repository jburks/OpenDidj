#!/bin/bash

# Build theora libs from source
THEORA_LIB_SRC=libtheora-1.0alpha7.tar.gz

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/theora/

if [ ! -e $THEORA_LIB_SRC ]; then
	wget http://downloads.xiph.org/releases/theora/$THEORA_LIB_SRC
fi

THEORA_LIB_DIR=`echo "$THEORA_LIB_SRC" | cut -d '.' -f -2`
echo $THEORA_LIB_DIR

if [ "$CLEAN" == "1" -o ! -e $THEORA_LIB_DIR ]; then
	rm -rf $THEORA_LIB_DIR
	tar -xzf $THEORA_LIB_SRC
fi

# build and copy shared libs to rootfs
pushd $THEORA_LIB_DIR
./configure --host=arm-linux --build=x86-linux --prefix=$ROOTFS_PATH/usr/local/lib --enable-shared=yes --disable-float --disable-encode --with-ogg=$ROOTFS_PATH/usr/local --with-ogg-includes=$ROOTFS_PATH/usr/local/include --with-ogg-libraries=$ROOTFS_PATH/usr/local/lib LDFLAGS="-L$ROOTFS_PATH/usr/local/lib -Wl,--rpath-link -Wl,$ROOTFS_PATH/usr/local/lib"
make
cp -a ./lib/.libs/libtheora.so* $ROOTFS_PATH/usr/local/lib/
cp -R ./include/theora $ROOTFS_PATH/usr/local/include/
popd

popd

exit 0
