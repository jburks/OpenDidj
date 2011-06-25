#!/bin/bash

# creates an initial Atomic Boot Flags partition JFFS2 image.

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# destination
OUTDIR=./bootflags

if [ "$CLEAN" == "1" ]; then
	rm -rf $OUTDIR
fi

mkdir -p $OUTDIR
echo "RFS0" > $OUTDIR/rootfs

# erase block size is larger for MLC NAND, padding affects ECC values
if [ "$TARGET_MACH" == "LF_MLC_LF1000" ]; then
	mkfs.jffs2 -n -l -e 256KiB -x zlib -x rtime -d $OUTDIR -o $OUTDIR.jffs2
else
	mkfs.jffs2 -n -l -p -e 128KiB -x zlib -x rtime -d $OUTDIR -o $OUTDIR.jffs2
fi

cp $OUTDIR.jffs2 $TFTP_PATH/

exit 0

