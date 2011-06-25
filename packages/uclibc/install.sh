#!/bin/bash

# eventually, this will be replaced by a legit build of uclibc.  For now we
# pull stuff from the scratchbox install.

#UCLIBC_PATH=/scratchbox/compilers/arm-gcc4.1-uclibc20061004/
UCLIBC_PATH=/opt/RidgeRun/arm-eabi-uclibc/
set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*


pushd $UCLIBC_PATH/lib
cp -a *.so* $ROOTFS_PATH/lib
chmod a+x $ROOTFS_PATH/lib/ld-uClibc-0.9.29.so
popd

if [ $EMBEDDED -eq 0 ]; then
	pushd $UCLIBC_PATH/include
	mkdir -p $ROOTFS_PATH/usr/include/asm/arch
	cp *.h $ROOTFS_PATH/usr/include/
	cp asm/*.h $ROOTFS_PATH/usr/include/asm
	cp asm/arch/*.h $ROOTFS_PATH/usr/include/asm/arch
	popd
fi

exit 0
