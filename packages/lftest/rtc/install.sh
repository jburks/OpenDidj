#!/bin/bash

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/lftest/rtc

cp lftest_rtc $ROOTFS_PATH/usr/bin

GCC=gcc
INCLUDES=-I$PROJECT_PATH/linux-2.6.20-lf1000/include
$CROSS_COMPILE$GCC $INCLUDES -o rtctest rtctest.c
cp rtctest $ROOTFS_PATH/usr/bin

popd

exit 0
