#!/bin/bash

ALSA_SRC=alsa-utils-1.0.13.tar.bz2
ALSA_LIB_SRC=alsa-lib-1.0.13.tar.bz2

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/alsa/

if [ ! -e $ALSA_SRC ]; then
	wget ftp://ftp.alsa-project.org/pub/utils/$ALSA_SRC
fi

if [ ! -e $ALSA_LIB_SRC ]; then
	wget ftp://ftp.alsa-project.org/pub/lib/$ALSA_LIB_SRC
fi

ALSA_DIR=`echo "$ALSA_SRC" | cut -d '.' -f -3`
ALSA_LIB_DIR=`echo "$ALSA_LIB_SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $ALSA_LIB_DIR ]; then
	rm -rf $ALSA_LIB_DIR
	tar -xjf $ALSA_LIB_SRC
fi

if [ "$CLEAN" == "1" -o ! -e $ALSA_DIR ]; then
	rm -rf $ALSA_DIR
	tar -xjf $ALSA_SRC
fi

# build and copy over libasound
pushd $ALSA_LIB_DIR
./configure --host=arm-linux --enable-shared=yes
make
cp -a ./src/.libs/libasound.so* $ROOTFS_PATH/usr/lib/
popd

# build and copy over alsa user tools
pushd $ALSA_DIR
./configure --host=arm-linux --with-shared --disable-alsamixer --disable-alsatest CPPFLAGS=-I$PROJECT_PATH/packages/alsa/$ALSA_LIB_DIR/include LDFLAGS=-L$PROJECT_PATH/packages/alsa/$ALSA_LIB_DIR/src/.libs

make
cp aplay/aplay $ROOTFS_PATH/usr/bin/
cp alsactl/alsactl $ROOTFS_PATH/usr/bin/
cp amixer/amixer $ROOTFS_PATH/usr/bin/
cp speaker-test/speaker-test $ROOTFS_PATH/usr/bin/

popd

# copy over alsa config
mkdir -p $ROOTFS_PATH/usr/share/alsa/
svn export --force ./config $ROOTFS_PATH/usr/share/alsa/

popd

exit 0
