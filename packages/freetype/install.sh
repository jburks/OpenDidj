#!/bin/bash

# Build freetype libs from source
FREETYPE_LIB_SRC=freetype-2.3.4.tar.gz

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/freetype/

if [ ! -e $FREETYPE_LIB_SRC ]; then
	wget http://download.savannah.gnu.org/releases/freetype/$FREETYPE_LIB_SRC
fi

FREETYPE_LIB_DIR=`echo "$FREETYPE_LIB_SRC" | cut -d '.' -f -3`
echo $FREETYPE_LIB_DIR

if [ "$CLEAN" == "1" -o ! -e $FREETYPE_LIB_DIR ]; then
	rm -rf $FREETYPE_LIB_DIR
	tar -xzf $FREETYPE_LIB_SRC
fi

# build and copy shared libs to rootfs
pushd $FREETYPE_LIB_DIR
./configure --host=arm-linux --build=x86-linux --prefix=$ROOTFS_PATH/usr/local --enable-shared=yes
make
# make install
cp -a ./objs/.libs/libfreetype.so* $ROOTFS_PATH/usr/local/lib/
cp -R ./include/freetype $ROOTFS_PATH/usr/local/include/
popd

popd

exit 0
