#!/bin/bash

SRC=tree-1.5.1.1.tgz
SRC_URL=ftp://mama.indstate.edu/linux/tree

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

#parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/tree

if [ ! -e $SRC ]; then
	wget $SRC_URL/$SRC
fi

BUILD_DIR=`echo "$SRC" | cut -d '.' -f -4`
echo "BUILD_DIR=" $BUILD_DIR

if [ "$CLEAN" == "1" -o ! -e $BUILD_DIR ]; then
	rm -rf $BUILD_DIR
	tar -xvzf $SRC
fi

pushd $BUILD_DIR

prefix=$ROOTFS_PATH
MANDIR=/tmp/tree
make CC="$CROSS_COMPILE"gcc
cp tree $ROOTFS_PATH/usr/bin
rm -rf $MANDIR
popd

popd

exit 0
