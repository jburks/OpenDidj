#!/bin/bash

set -e
MTD_SRC=mtd-utils-20070911.tar.gz
ZLIB_SRC=zlib-1.2.3
LZO_SRC=lzo-2.02
ARGP_SRC=argp-standalone-1.3

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/mtd-utils

MTD_DIR=mtd-utils

if [ "$CLEAN" == "1" -o ! -e $MTD_DIR ]; then
	rm -rf $MTD_DIR
	tar -xzf $MTD_SRC
	pushd $MTD_DIR
	# we need to be able to set the include path using CFLAGS
	patch -p1 < ../cflags-and-standalone-argp.patch
	patch -p1 < ../md5-option-for-nanddump.patch
	patch -p1 < ../nand-dump-md5-fix.patch
	# make nandtest not stop on errors and be more verbose in stderr
	patch -p0 < ../nandtest_dont_stop_on_errors.patch
	patch -p0 < ../nandtest_ECC_failed.patch
	patch -p1 < ../mtd-bbt-ioctls.patch
	popd
fi

pushd $MTD_DIR

INCDIR=$PROJECT_PATH/packages/zlib/$ZLIB_SRC/
if [ ! -e $INCDIR ]; then
	echo "Can't find zlib at $INCDIR.  Did you build zlib?"
	exit 1
fi

LZOINC=$PROJECT_PATH/packages/lzo/$LZO_SRC/include
LZOLIB=$PROJECT_PATH/packages/lzo/$LZO_SRC/src/.libs/
if [ ! -e $LZOINC ]; then
	echo "Can't find lzo at $LZODIR.  Did you build lzo?"
	exit 1
fi

ARGPINC=$PROJECT_PATH/packages/argp/$ARGP_SRC/
ARGPLIB=$PROJECT_PATH/packages/argp/$ARGP_SRC/
if [ ! -e $ARGPINC ]; then
	echo "Can't find argp at $ARGPDIR.  Did you build argp?"
	exit 1
fi

CROSS=$CROSS_COMPILE CFLAGS="-I./src -I$INCDIR -I$LZOINC -I$ARGPINC" LDFLAGS="-L$INCDIR -L$LZOLIB -L$ARGPLIB" make WITHOUT_XATTR=1 DESTDIR=$ROOTFS_PATH install

popd

# Create profnand.c and put in /usr/bin
GCC=gcc
INCLUDES="$INCLUDE -Imtd-utils/include -Imtd-utils/ubi-utils/src"
$CROSS_COMPILE$GCC $INCLUDES -o profnand profnand.c
$CROSS_COMPILE$GCC $INCLUDES -o nandscrub nandscrub.c
$CROSS_COMPILE$GCC $INCLUDES -o nandwipebbt nandwipebbt.c
$CROSS_COMPILE$GCC $INCLUDES -o nandscan nandscan.c
cp -pv profnand nandscrub nandscan nandwipebbt $ROOTFS_PATH/usr/bin

popd

exit 0
