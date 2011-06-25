#!/bin/bash

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/lftest/gpio

# (test is obsolete)
#cp lftest_gpio $ROOTFS_PATH/usr/bin

GCC=gcc
INCLUDES=-I$PROJECT_PATH/linux-2.6.20-lf1000/include
$CROSS_COMPILE$GCC $INCLUDES -o gpio-control gpio-control.c
cp gpio-control $ROOTFS_PATH/usr/bin

popd

exit 0
