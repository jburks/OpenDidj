#!/bin/bash

# This script sets up a root file system with device nodes and then populates
# it with the various packages.  Pass the -b option if you wish to build brio.
# Pass the -c option to do a clean build.

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

if [ $EMBEDDED -eq 1 ]; then
	echo "Creating embedded filesystem"
	OLD_ROOTFS_PATH=$ROOTFS_PATH
	export ROOTFS_PATH=$EROOTFS_PATH
	if [ ! -d $ROOTFS_PATH ] ; then
		echo "Creating embedded root filesystem staging area $ROOTFS_PATH"
		mkdir $ROOTFS_PATH
	else
		echo "Embedded root filesystem staging area $ROOTFS_PATH already exists"
        fi
else
	echo "Creating development filesystem"
fi

if [ "$CLEAN" == "1" ]; then
	# we have to be root to remove the files that oprofile created.  Yuck.
	sudo rm -rf $ROOTFS_PATH/var/lib/oprofile
	sudo rm -rf $ROOTFS_PATH/root/.oprofile
	rm -rf $ROOTFS_PATH
fi

# Create directory structure
echo "****************************************************"
echo "setting up directories..."
$PROJECT_PATH/scripts/make_directories.sh $*

# Add kernel modules and standard device nodes
echo "****************************************************"
echo "Creating kernel and device nodes"
pushd $PROJECT_PATH/linux-2.6/
./install.sh $*
popd

# calculate package list
if [ $EMBEDDED -eq 0 ]; then
	PACKAGES=$PROJECT_PATH/scripts/complete-package-list
else
	PACKAGES=$PROJECT_PATH/scripts/embedded-package-list
fi

PKG_PREFIX=$PROJECT_PATH/packages
while read package; do
	set +e
	COMMENT=`echo "$package" | egrep "^#[.\w]*"`
	set -e

	if [ "$COMMENT" != "" ]; then
		continue
	fi

	echo "****************************************************"
	echo "Installing $package..."

	if [ ! -e "$PKG_PREFIX/$package/install.sh" ]; then
		echo "Package $package does not have an install.sh script."
		exit 1;
	fi

	$PKG_PREFIX/$package/install.sh $*
done < $PACKAGES

echo "****************************************************"

if [ "$BRIO" == "1" ]; then
	# make Brio and LightningCore Base
	$PROJECT_PATH/scripts/make_brio.sh $*
	$PROJECT_PATH/scripts/make_base.sh $*
fi

if [ "$EMBEDDED" -eq "1" ]; then
	rm -R $ROOTFS_PATH/usr/include
	chmod a+rwx $ROOTFS_PATH/lib/libuClibc-0.9.29.so
	#find $ROOTFS_PATH -type f | xargs md5sum > $ROOTFS_PATH/etc/md5sums
	if [ "$TARGET_MACH" == "LF_MLC_LF1000" ]; then
		ERASE_SIZE=256KiB
	else
		ERASE_SIZE=128KiB
	fi
	echo "Making erootfs.jffs2 image, erase size = $ERASE_SIZE"
	mkfs.jffs2 -l -n -e $ERASE_SIZE -p -o $ROOTFS_PATH/../erootfs-tmp.jffs2 -d $ROOTFS_PATH
	sumtool -l -n -e $ERASE_SIZE -p -i $ROOTFS_PATH/../erootfs-tmp.jffs2 -o $ROOTFS_PATH/../erootfs.jffs2
	rm $ROOTFS_PATH/../erootfs-tmp.jffs2
	ROOTFS_PATH=$OLD_ROOTFS_PATH
	export ROOTFS_PATH
fi

echo "done"

exit 0

