#!/bin/bash

# creates an uncompressed Kernel partition JFFS2 image, containint a uImage and
# (if needed) a u-boot image

set -e

echo "WARNING: $0 is deprecated, use make_kernel.py instead."
exit 1

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# destination
OUTDIR=./kernel

if [ "$CLEAN" == "1" ]; then
	rm -rf $OUTDIR
fi

mkdir -p $OUTDIR

# copy over a kernel image
if [ ! -e $PROJECT_PATH/linux-2.6.20-lf1000/arch/arm/boot/uImage ]; then
	echo "didn't find kernel uImage, build your kernel first"
	exit 1
fi
cp $PROJECT_PATH/linux-2.6.20-lf1000/arch/arm/boot/uImage $OUTDIR/uImage

# copy over u-boot, if needed
if [ "$UBOOTLOADERS" == "1" ]; then
	if [ ! -e $PROJECT_PATH/u-boot-1.1.6-lf1000/u-boot.bin ]; then
		echo "didn't find u-boot image, build u-boot first"
		exit 1
	fi
	cp $PROJECT_PATH/u-boot-1.1.6-lf1000/u-boot.bin $OUTDIR/
fi

#find $OUTDIR -type f | xargs md5sum > $OUTDIR/md5sums

# copy over boot splash
if [ ! -e $PROJECT_PATH/packages/screens/bootsplash.rgb ]; then
	echo "WARNING: didn't find bootsplash.rgb"
else
	cp $PROJECT_PATH/packages/screens/bootsplash.rgb $OUTDIR/
fi

mkfs.jffs2 -n -l -p -e 128KiB -x zlib -x rtime -d $OUTDIR/ -o $OUTDIR.jffs2
du -h $OUTDIR.jffs2

cp $OUTDIR.jffs2 $TFTP_PATH/

exit 0

