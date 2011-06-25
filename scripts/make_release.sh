#!/bin/bash
# Create a release.  Put it in RELEASE_PATH

set -e
. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse the args
set_standard_opts $*

if [ $HELP == 1 ]; then
	echo "Usage $0 [options]"
	echo ""
	echo "Options:"
	echo "-h		Print help then quit"
	echo "-b		Add Brio to the release"
	echo "-e		Build the embedded release (jffs2 image)"
	echo "-u		Add u-boot to the release"
	exit 0
fi

if [ "X$RELEASE_PATH" == "X" ]; then
	export RELEASE_PATH=/tmp/lightning-release/
	echo "RELEASE_PATH not set.  Using default $RELEASE_PATH"
fi

# create the release dir
mkdir -p $RELEASE_PATH

# Get the version number from the version package
ROOTFS_PATH=$RELEASE_PATH $PROJECT_PATH/packages/version/install.sh
VERSION=`cat $RELEASE_PATH/etc/version`
rm -rf $RELEASE_PATH/etc/
LINUX_DIST_NAME=LinuxDist-$VERSION
LINUX_DIST_DIR=$RELEASE_PATH/$LINUX_DIST_NAME
mkdir -p $LINUX_DIST_DIR

echo "********* Creating Release $LINUX_DIST_NAME *********"

# make the rootfs and package it up
KERNEL_CONFIG_PATH=$PROJECT_PATH/linux-2.6.20-lf1000/.config

# move user's .config out of the way (we build defconfig)
KERNEL_CONFIG_MOVED=0
if [ -e $KERNEL_CONFIG_PATH ]; then
	mv $KERNEL_CONFIG_PATH $KERNEL_CONFIG_PATH.current
	KERNEL_CONFIG_MOVED=1
fi

# We only need one nfsroot image for any LF1000 target
ROOTFS_RELEASE=$VERSION-$TARGET_MACH
ROOTFS_PATH=$LINUX_DIST_DIR/nfsroot-$VERSION $PROJECT_PATH/scripts/make_rootfs.sh $*
ROOTFS_PATH=$LINUX_DIST_DIR/erootfs-$ROOTFS_RELEASE $PROJECT_PATH/scripts/make_rootfs.sh $* -e

# move user's .config back, if we had moved it out of the way
if [ $KERNEL_CONFIG_MOVED -eq 1 ]; then
	mv $KERNEL_CONFIG_PATH.current $KERNEL_CONFIG_PATH
fi

# build u-boot if needed
if [ "$UBOOTLOADERS" == "1" ]; then
	$PROJECT_PATH/u-boot-1.1.6-lf1000/install.sh
fi

# Create lfp deliverables of kernel and bootloader
mkdir -p $LINUX_DIST_DIR/packages
$PROJECT_PATH/scripts/make_lfps.sh $*
LFP_VERSION=`echo $VERSION | tr - .`
cp firmware-$TARGET_MACH-$LFP_VERSION.lfp $LINUX_DIST_DIR/packages/
cp bootstrap-$TARGET_MACH-$LFP_VERSION.lfp $LINUX_DIST_DIR/packages/

# Create lfp deliverables of Brio and Base binaries
mkdir -p $LINUX_DIST_DIR/packages/base
if [ "$BRIO" == "1" ]; then
	cp $LINUX_DIST_DIR/nfsroot-$VERSION/*.lfp $LINUX_DIST_DIR/packages/base/
fi

# Create MD5 and SHA1 checksums for packages
pushd $LINUX_DIST_DIR/packages/
md5sum *.lfp > md5sum.txt
sha1sum *.lfp > sha1sum.txt
popd
if [ "$BRIO" == "1" ]; then
	pushd $LINUX_DIST_DIR/packages/base/
	md5sum *.lfp > md5sum.txt
	sha1sum *.lfp > sha1sum.txt
	popd
fi

# Move the release notes over
cp $PROJECT_PATH/RELEASE-NOTES $LINUX_DIST_DIR/RELEASE-NOTES

# Move the host tools (and NAND Flash memory map) over
pushd $PROJECT_PATH
svn export --force host_tools $LINUX_DIST_DIR/host_tools
cp packages/lfpkg/lfpkg $LINUX_DIST_DIR/host_tools
scripts/make_map.py > $LINUX_DIST_DIR/host_tools/flash.map
popd

# Move over UART bootstrapping tools
mkdir -p $LINUX_DIST_DIR/uart_bootstrap
if [ "$TARGET_MACH" == "LF_MP2530F" ]; then
	cp $PROJECT_PATH/images/boot-u.nb0 $LINUX_DIST_DIR/uart_bootstrap/
else
	cp $PROJECT_PATH/images/UARTBOOT.bin $LINUX_DIST_DIR/uart_bootstrap/
fi

# provide u-boot for UART bootstrap
if [ "$UBOOTLOADERS" == "1" ]; then
	cp $PROJECT_PATH/u-boot-1.1.6-lf1000/u-boot.bin $LINUX_DIST_DIR/uart_bootstrap/u-boot-$VERSION-$TARGET_MACH.bin
fi

echo "This directory contains images needed for bootstrapping a bricked (or brand new) board via the UART boot process.  They may be used with lf1000_bootstrap.py or mp2530_bootstrap.py (see the host_tools README for instructions).  You do NOT need these images when doing normal firmware upgrades or installing releases." > $LINUX_DIST_DIR/uart_bootstrap/README

# Move ATAP manufacturing cartridge tools over
pushd $PROJECT_PATH
mkdir -p $LINUX_DIST_DIR/mfg-cart
mkdir -p $LINUX_DIST_DIR/mfg-cart/ATAP
svn export --force  packages/mfg-cart/ATAP $LINUX_DIST_DIR/mfg-cart/ATAP
#cp -Rf packages/mfg-cart/ATAP/* $LINUX_DIST_DIR/mfg-cart/ATAP
#rm -rf $LINUX_DIST_DIR/mfg-cart/ATAP/.svn
cp -f $LINUX_DIST_DIR/packages/*.lfp $LINUX_DIST_DIR/mfg-cart/ATAP/FW_packages
if [ "$BRIO" == "1" ]; then
	cp -f $LINUX_DIST_DIR/packages/base/*.lfp $LINUX_DIST_DIR/mfg-cart/ATAP/Packages
fi
cp -f $LINUX_DIST_DIR/host_tools/lfpkg $LINUX_DIST_DIR/mfg-cart/ATAP
cp -f $LINUX_DIST_DIR/nfsroot-$VERSION/usr/bin/mkbase.sh $LINUX_DIST_DIR/mfg-cart/ATAP
#tar -czf ATAP-$VERSION.tar.gz $LINUX_DIST_DIR/mfg-cart/ATAP
#mv ATAP*.gz $LINUX_DIST_DIR/mfg-cart
svn export --force  packages/mfg-cart/Base2Cart $LINUX_DIST_DIR/mfg-cart/Base2Cart
cp -f $LINUX_DIST_DIR/host_tools/lfpkg $LINUX_DIST_DIR/mfg-cart/Base2Cart
svn export --force  packages/mfg-cart/Base2ATAP $LINUX_DIST_DIR/mfg-cart/Base2ATAP
cp -f $LINUX_DIST_DIR/host_tools/lfpkg $LINUX_DIST_DIR/mfg-cart/Base2ATAP
cp -f $LINUX_DIST_DIR/nfsroot-$VERSION/usr/bin/mkbase.sh $LINUX_DIST_DIR/mfg-cart/Base2ATAP
CUR_PATH="`pwd`"
cd $LINUX_DIST_DIR/mfg-cart
../host_tools/lfpkg -a create Base2Cart
../host_tools/lfpkg -a create Base2ATAP
rm -rf Base2Cart
rm -rf Base2ATAP
#mv Base2*.lfp $LINUX_DIST_DIR/mfg-cart
cd $CUR_PATH
popd

# tar up the release
pushd $RELEASE_PATH
sudo tar -czf $LINUX_DIST_NAME.tar.gz $LINUX_DIST_NAME
popd

exit 0
