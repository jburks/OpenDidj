#!/bin/bash

# Build vorbis libs from source
# (Floating-point version not used except for reference)
VORBIS_LIB_SRC=libvorbis-1.1.2.tar.gz

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/vorbis/

if [ ! -e $VORBIS_LIB_SRC ]; then
	wget http://downloads.xiph.org/releases/vorbis/$VORBIS_LIB_SRC
fi

VORBIS_LIB_DIR=`echo "$VORBIS_LIB_SRC" | cut -d '.' -f -3`
echo $VORBIS_LIB_DIR

if [ "$CLEAN" == "1" -o ! -e $VORBIS_LIB_DIR ]; then
	rm -rf $VORBIS_LIB_DIR
	tar -xzf $VORBIS_LIB_SRC
fi

# build and copy shared libs to rootfs
pushd $VORBIS_LIB_DIR
./configure --host=arm-linux --build=x86-linux --prefix=$ROOTFS_PATH/usr/local --enable-shared=yes --with-ogg=$ROOTFS_PATH/usr/local LDFLAGS="-L$ROOTFS_PATH/usr/local/lib -Wl,--rpath-link -Wl,$ROOTFS_PATH/usr/local/lib" 
make
# make install
cp -a ./lib/.libs/libvorbis.so* $ROOTFS_PATH/usr/local/lib/
cp -R ./include/vorbis $ROOTFS_PATH/usr/local/include/
popd

popd

exit 0
