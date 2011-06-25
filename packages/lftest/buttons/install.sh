#!/bin/bash

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/lftest/buttons

cp lftest_buttons $ROOTFS_PATH/usr/bin

GCC=gcc
INCLUDES=-I$PROJECT_PATH/linux-2.6.20-lf1000/include
# evtest for testing /dev/input/eventX interface
$CROSS_COMPILE$GCC $INCLUDES -o evtest evtest.c
cp evtest $ROOTFS_PATH/usr/bin
$CROSS_COMPILE$GCC $INCLUDES -o keyboard-example keyboard-example.c
cp keyboard-example $ROOTFS_PATH/usr/bin
$CROSS_COMPILE$GCC $INCLUDES -o switches-example switches-example.c
cp switches-example $ROOTFS_PATH/usr/bin
popd

exit 0
