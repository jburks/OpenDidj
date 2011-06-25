#!/bin/bash

# check if we can use sparse
which sparse > /dev/null
if [ "$?" = "0" ]; then
	SPARSE_CHECK="C=1"
else
	echo "**** WARNING: you do not have sparse installed"
	echo "**** run: \"sudo apt-get install sparse\" to install it"
	echo ""
	SPARSE_CHECK=""
fi

# count number of CPUs for make

if [ -z "$CPUS" ]
then
  export CPUS=$[$(echo /sys/devices/system/cpu/cpu[0-9]* | wc -w)+0]
  [ "$CPUS" -lt 1 ] && CPUS=1
fi

set -e

echo "linux-2.6/install.sh: BEFORE functions TARGET_MACH=$TARGET_MACH"
. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

echo "linux-2.6/install.sh: AFTER functions TARGET_MACH=$TARGET_MACH"

pushd $PROJECT_PATH/linux-2.6/

# check for .config.  Use defconfig if it's not there.
if [ ! -e ".config" ]; then
	echo "*** Using default OpenDidj configuration"
	make -j $CPUS opendidj_defconfig
fi

# Apply patches; -N ignore already applied, -r - don't make rejects file
set +e
patch -r - -p0 -N < default-log-level.patch
patch -r - -p0 -N < rtc-laxity.patch
set -e

make $SPARSE_CHECK -j $CPUS zImage
cp arch/arm/boot/zImage $TFTP_PATH

MODS=`grep CONFIG_MODULES include/config/auto.conf`
if [ "$MODS" != "" ]; then
	make -j $CPUS modules
	echo INSTALL_MOD_PATH=$ROOTFS_PATH
	make -j $CPUS modules_install INSTALL_MOD_PATH=$ROOTFS_PATH
fi

# create linux headers
make headers_install ARCH=arm INSTALL_HDR_PATH=$ROOTFS_PATH/usr

if [ $EMBEDDED -eq 0 ]; then
	cp vmlinux $ROOTFS_PATH
fi

# determine the platform we've built for
MACH="`grep CONFIG_MACH include/linux/autoconf.h | cut -d ' ' -f 2`"
NFSROOT="`grep CONFIG_ROOT_NFS include/linux/autoconf.h | cut -d ' ' -f 2`"
MFGCART="`grep prg_boot include/linux/autoconf.h | cut -d ' ' -f 2`"

# pick debug console based on platform/board
case $MACH in
	"CONFIG_MACH_ME_LF1000")
		CONSOLE="ttyS0"
		;;
	"CONFIG_MACH_LF_LF1000")
		CONSOLE="ttyS0"
		;;
	*)
		CONSOLE="ttyS0"
		;;
esac

popd

#### Don't do most device node creation now ###
#### See /etc/init.d/rcS and /usr/bin/make_device_nodes.sh; 
#### We do most of this at run time now.  
#### Exception is /dev/console, which init wants to grab before rcS runs, so
#### if we want to watch it boot, we'll have to have a /dev/console before
#### /dev is mounted
mkdir -p $ROOTFS_PATH/dev
pushd $ROOTFS_PATH/dev
rm -f ttyS0
sudo mknod -m 600 ttyS0 c 4 64
ln -sf $CONSOLE console
# popd

# # Create device nodes.  If your package requires additional device nodes, your
# # package's install script should create them.
# echo "making device nodes..."
# mkdir -p $ROOTFS_PATH/dev
# pushd $ROOTFS_PATH/dev

# mkdev perm name type major minor.  For example:
# mkdev 600 mem c 1 1
function mkdev() {
	if [ -e $2 ]; then
		rm -f $2
	fi
	sudo mknod -m $1 $2 $3 $4 $5
}

mkdev 666 mem c 1 1
mkdev 666 null c 1 3
mkdev 666 zero c 1 5
mkdev 644 random c 1 8
mkdev 644 urandom c 1 9
mkdev 600 tty0 c 4 0
mkdev 600 tty1 c 4 1
mkdev 600 tty2 c 4 2
mkdev 600 tty3 c 4 3
mkdev 600 ttyS0 c 4 64
mkdev 600 ttyS1 c 4 65
mkdev 600 ttyS2 c 4 66
mkdev 600 ttyS3 c 4 67
mkdev 666 tty c 5 0
# mkdev 666 watchdog c 10 130
mkdev 644 video0 c 81 0

# # make sure console points to the right serial port
# ln -sf $CONSOLE console

# # These are devices for the custom lf1000 devices
mkdev 666 gpio c 246 0
# #mkdev 666 buttons c 247 0
mkdev 666 mlc c 247 0
# # mkdev 666 power c 239 0

# if [ $MACH == "CONFIG_MACH_ME_LF1000" ]; then
# 	M_LAYERS=2
# else
# 	M_LAYERS=3
# fi
# for i in `seq 0 $M_LAYERS`; do
# 	mkdev 666 layer$i c 245 $i
# done

# mkdev 666 dpc c 252 0
# mkdev 666 ga3d c 249 0

# # Add device node for /dev/idct, IDCT Macro Block Decoder for Video.
# mkdev 666 idct c 248 0

# # Add devices for the mtd subsystem.
for m in `seq 0 9`; do
	mkdev 666 mtd$m c 90 $((2*$m))
# 	# mkdev 666 mtdr$m c 90 $((2*$m+1));
	mkdev 666 mtdblock$m b 31 $m;
done

# # make device node for OSS audio
mkdev 666 dsp c 14 3

# # make device node for i2c:
# # (do this if you want to use the i2c chardev driver, i2c-dev, otherwise it is
# #  not needed)
# #
mkdev 666 i2c-0 c 89 0

# # make device node for SPI:
# # (do this if you want to use the SPI driver from user space, otherwise it is 
# # not needed)
# #
# # Note: I stole 243 for the ubi layer.  Please don't use.
# # mkdev 666 spi c 243 0

# make device node for /dev/rtc0
mkdev 666 rtc0 c 254 0

# # make device node for /dev/ram0.  This is the ramdisk for USB
# mkdev 666 ram0 b 1 0

#make device node for /dev/power.  This driver monitors the battery
mkdev 666 power c 239 0

# # make device nodes for ubi.  When are we going to fix this stuff?
# # mkdev 666 ubi0 c 240 0;
# # mkdev 666 ubi1 c 241 0;
# # mkdev 666 ubi2 c 242 0;
# # mkdev 666 ubi3 c 243 0;
# mkdev 666 ubi_ctrl c 10 63;

# # keyboard event interface (plus touchscreen)
# # Create quite a few device nodes in case HID devices get plugged in.  In the
# # future, we'll hotplug instead and this won't be needed.
mkdir -p ./input
for i in `seq 0 19`; do
	mkdev 666 input/event$i c 13 $((64+$i))
done

# # make device nodes for Ethernet over USB / Telnet support if needed.
mkdev 666 ptmx c 5 2
mkdir -p pts
chmod 777 pts

popd

# stash copies of this build, identified by platform and root filesystem
if [ "$MACH" == "CONFIG_MACH_ME_LF1000" ] ; then
	if [ "X$NFSROOT" != "X" ] ; then
		echo "Saving build specific copies as $TFTP_PATH/uImage.nfs and $TFTP_PATH/zImage.nfs"
		# cp $TFTP_PATH/uImage $TFTP_PATH/uImage.nfs
		cp $TFTP_PATH/zImage $TFTP_PATH/zImage.nfs
	else
		echo "Saving build specific copies as $TFTP_PATH/uImage.nand and $TFTP_PATH/zImage.nand"
		# cp $TFTP_PATH/uImage $TFTP_PATH/uImage.nand
		cp $TFTP_PATH/zImage $TFTP_PATH/zImage.nand
	fi
elif [ "$MACH" == "CONFIG_MACH_LF_LF1000" ] ; then
	echo "Saving build specific copies as $TFTP_PATH/uImage.ff_bd and $TFTP_PATH/zImage.ff_bd"
	# cp $TFTP_PATH/uImage $TFTP_PATH/uImage.ff_bd
	cp $TFTP_PATH/zImage $TFTP_PATH/zImage.ff_bd
fi
