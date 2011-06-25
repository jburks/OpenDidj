#! /bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

echo "-------------------------------------------------------------------------"
echo "Rebuilding kernel for Form Factor board"
echo "-------------------------------------------------------------------------"

pushd $PROJECT_PATH/linux-2.6.20-lf1000/
make mrproper
make lf1000_ff_defconfig
./install.sh
popd

echo "-------------------------------------------------------------------------"
echo "Rebuilding Lightning Bootstrap"
echo "-------------------------------------------------------------------------"

pushd $PROJECT_PATH/lightning-boot/
make clean
./install.sh
popd

echo "-------------------------------------------------------------------------"
echo "Rebuilding U-Boot"
echo "-------------------------------------------------------------------------"

pushd $PROJECT_PATH/u-boot-1.1.6-lf1000/
make clean
./install.sh
popd

pushd $PROJECT_PATH/scripts/

echo "-------------------------------------------------------------------------"
echo "Building bootflags filesystem"
echo "-------------------------------------------------------------------------"

./make_bootflags.sh

echo "-------------------------------------------------------------------------"
echo "Building kernel filesystem"
echo "-------------------------------------------------------------------------"

./make_kernel.sh -u

echo "-------------------------------------------------------------------------"
echo "Building root filesystem"
echo "-------------------------------------------------------------------------"

./make_rootfs.sh -e -c
popd

if [ ! -d $PROJECT_PATH/scripts/firmware ] ; then
    mkdir -p $PROJECT_PATH/scripts/firmware
fi

pushd $EROOTFS_PATH
find . -type f | xargs md5sum > $PROJECT_PATH/scripts/firmware/md5sums
popd

echo "-------------------------------------------------------------------------"
echo "Copying firmware files to $PROJECT_PATH/scripts/firmware"
echo "-------------------------------------------------------------------------"

pushd $PROJECT_PATH/scripts/firmware
cp $TFTP_PATH/lightning-boot.bin .
cp $PROJECT_PATH/scripts/bootflags.jffs2 .
cp $PROJECT_PATH/scripts/kernel.jffs2 .
cp $PROJECT_PATH/packages/lfpkg/firmware/meta.inf .
cp $PROJECT_PATH/packages/mfg-cart/mkbase.sh .
cp $PROJECT_PATH/packages/mfg-cart/mfg-start.sh .
cp $ROOTFS_PATH/../erootfs.jffs2 .
popd

echo "-------------------------------------------------------------------------"
echo "Building package from files in $PROJECT_PATH/scripts/firmware"
echo "-------------------------------------------------------------------------"

pushd $PROJECT_PATH/scripts
$PROJECT_PATH/packages/lfpkg/lfpkg -a create -d firmware
popd
