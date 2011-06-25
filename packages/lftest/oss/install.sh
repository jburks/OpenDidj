#!/bin/bash

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/lftest/oss

cp lftest_oss $ROOTFS_PATH/usr/bin

GCC=gcc
INCLUDES=-I$PROJECT_PATH/linux-2.6.20-lf1000/include
$CROSS_COMPILE$GCC $INCLUDES -o oss oss.c
chmod a+rwx ./oss
cp oss $ROOTFS_PATH/usr/bin
$CROSS_COMPILE$GCC $INCLUDES -o vol vol.c
chmod a+rwx ./vol
cp vol $ROOTFS_PATH/usr/bin
mkdir -p $ROOTFS_PATH/test
cp vivaldi.wav $ROOTFS_PATH/test/

popd

exit 0
