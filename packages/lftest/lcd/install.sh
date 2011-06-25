#!/bin/bash
set -e

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/lftest/lcd

cp lftest_lcd $ROOTFS_PATH/usr/bin

GCC=gcc
INCLUDES="-I$PROJECT_PATH/linux-2.6.20-lf1000/include -I$PROJECT_PATH/packages/libpng/libpng-1.2.22/ -I$PROJECT_PATH/packages/zlib/zlib-1.2.3/"
$CROSS_COMPILE$GCC $INCLUDES -o dpc-control dpc-control.c
cp dpc-control $ROOTFS_PATH/usr/bin
$CROSS_COMPILE$GCC $INCLUDES -o mlc-control mlc-control.c
cp mlc-control $ROOTFS_PATH/usr/bin
$CROSS_COMPILE$GCC $INCLUDES -o layer-control layer-control.c
cp layer-control $ROOTFS_PATH/usr/bin
$CROSS_COMPILE$GCC $INCLUDES -L$PROJECT_PATH/packages/libpng/libpng-1.2.22/.libs/ -L$PROJECT_PATH/packages/zlib/zlib-1.2.3/ -o imager readpng.c imager.c -lpng12 -lz
cp imager $ROOTFS_PATH/usr/bin
cp test-rgb.sh $ROOTFS_PATH/usr/bin
$CROSS_COMPILE$GCC $INCLUDES -o drawtext drawtext.c
cp drawtext $ROOTFS_PATH/usr/bin
mkdir -p $ROOTFS_PATH/test
cp testimg.rgb $ROOTFS_PATH/test/
cp monotext8x16.rgb $ROOTFS_PATH/test

popd

exit 0
