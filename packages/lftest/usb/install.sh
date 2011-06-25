#!/bin/bash

. $PROJECT_PATH/scripts/functions

check_vars

pushd $PROJECT_PATH/packages/lftest/usb

cp lftest_usb $ROOTFS_PATH/usr/bin

popd

exit 0
