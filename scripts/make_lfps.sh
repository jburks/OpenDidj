#!/bin/bash

# creates the lfps.  The -u option forces the bootloader lfp to be created too.

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

if [ "$CLEAN" = "1" ]; then
	rm -rf bootstrap
	rm -rf firmware
fi

# Get version number
ROOTFS_PATH=$PWD $PROJECT_PATH/packages/version/install.sh
VERSION=`cat $PWD/etc/version`
rm -rf $PWD/etc/

mkdir -p bootstrap-$TARGET_MACH
$PROJECT_PATH/lightning-boot/install.sh $*
cp $PROJECT_PATH/lightning-boot/lightning-boot.bin bootstrap-$TARGET_MACH/

$PROJECT_PATH/scripts/make_bootflags.sh $*
cp bootflags.jffs2 bootstrap-$TARGET_MACH/

echo -e \
Device=\"Didj\"\\n\
Type=\"System\"\\n\
Name=\"Didj Device Bootstrap\"\\n\
Publisher=\"LeapFrog, Inc.\"\\n\
Developer=\"LeapFrog, Inc.  Firmware Team\"\\n\
Version=\"`echo $VERSION | tr - .`\"\\n\
ProductID=0x000E0002\\n\
PackageID=\"DIDJ-0x000E0002-000001\"\\n\
PartNumber=\"152-12221\"\\n\
Hidden=1\\n\
ProductIDDepends=\\n\
Depends=\\n\
MetaVersion=\"1.0\"\\n\
Icon= \\n\
Locale=\"en-us\"\\n\
> bootstrap-$TARGET_MACH/meta.inf
md5sum bootstrap-$TARGET_MACH/lightning-boot.bin | awk '{print $1}' > bootstrap-$TARGET_MACH/lightning-boot.md5
cp bootstrap-$TARGET_MACH/lightning-boot.md5 bootstrap-$TARGET_MACH/lightning-boot.bin.md5
$PROJECT_PATH/packages/lfpkg/lfpkg -a create bootstrap-$TARGET_MACH
rm -r bootstrap-$TARGET_MACH
	
# create the firmware package
mkdir -p firmware-$TARGET_MACH

$PROJECT_PATH/scripts/make_kernel.py $*

cp kernel.bin firmware-$TARGET_MACH/
cp $EROOTFS_PATH/../erootfs.jffs2 firmware-$TARGET_MACH/
echo -e \
Device=\"Didj\"\\n\
Type=\"System\"\\n\
Name=\"Didj Device Firmware\"\\n\
Publisher=\"LeapFrog, Inc.\"\\n\
Developer=\"LeapFrog, Inc.  Firmware Team\"\\n\
Version=\"`echo $VERSION | tr - .`\"\\n\
ProductID=0x000E0003\\n\
PackageID=\"DIDJ-0x000E0003-000001\"\\n\
PartNumber=\"152-12221\"\\n\
Hidden=1\\n\
Depends=\\n\
ProductIDDepends=\\n\
MetaVersion=\"1.0\"\\n\
Icon= \\n\
Locale=\"en-us\"\\n\
> firmware-$TARGET_MACH/meta.inf
md5sum firmware-$TARGET_MACH/erootfs.jffs2 | awk '{print $1}' > firmware-$TARGET_MACH/erootfs.md5
md5sum firmware-$TARGET_MACH/kernel.bin | awk '{print $1}' > firmware-$TARGET_MACH/kernel.md5

$PROJECT_PATH/packages/lfpkg/lfpkg -a create firmware-$TARGET_MACH
rm -r firmware-$TARGET_MACH

exit 0

