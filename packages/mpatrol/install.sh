#!/bin/bash

URL=http://www.cbmamiga.demon.co.uk/mpatrol/files/
SRC=mpatrol_1.4.8.tar.gz

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/mpatrol

if [ ! -e $SRC ]; then
	wget $URL/$SRC
fi

BUILD_DIR=mpatrol

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xzf $SRC
	pushd $BUILD_DIR
	patch -p1 < ../cross-compile.patch
	popd
fi

pushd $BUILD_DIR/build/unix
make all
PREFIX=$ROOTFS_PATH/usr/ make install
popd

popd

exit 0