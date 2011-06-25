#!/bin/bash

BUSYBOX_SRC=busybox-1.5.0.tar.bz2

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/busybox

if [ ! -e $BUSYBOX_SRC ]; then
	wget http://www.busybox.net/downloads/$BUSYBOX_SRC
fi

BUSYBOX_DIR=`echo "$BUSYBOX_SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" -o ! -e $BUSYBOX_DIR ]; then
	rm -rf $BUSYBOX_DIR
	tar -xjf $BUSYBOX_SRC
fi

patch -p 0 < readbug.patch

pushd $BUSYBOX_DIR
cp ../busybox.config ./.config

make TARGET_ARCH=arm CROSS_COMPILE=$CROSS_COMPILE all install
cp -ra _install/* $ROOTFS_PATH
popd

popd

exit 0
