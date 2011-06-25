#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# Base is meant to be built on nfsroot image only, not erootfs image
if [ $EMBEDDED -eq 1 ]; then
	echo "Skipping LightningCore for $ROOTFS_PATH"
	exit 0
fi

echo "****************************************************"
echo "Building LightningCore onto $ROOTFS_PATH"

if [ "X$LIGHTNING_CORE_PATH" == "X" ]; then
	LIGHTNING_CORE_PATH=~/workspace/LightningCore/
	echo "LIGHTNING_CORE_PATH not set.  Using default $LIGHTNING_CORE_PATH"
fi

if [ ! -d $LIGHTNING_CORE_PATH ]; then
	echo "Error: LIGHTNING_CORE_PATH $LIGHTNING_CORE_PATH does not exist."
	echo "Please set LIGHTNING_CORE_PATH environment variable."
	exit 1
fi

pushd $LIGHTNING_CORE_PATH

# check for L3X touchscreen platform variant
if [ "$TARGET_MACH" == "LF_TS_LF1000" -o "$TARGET_MACH" == "LF_TS_64MB_LF1000" ]; then
	VARIANT='l3x'
else
	VARIANT='didj'
fi

if [ "$CLEAN" == "1" ]; then
	scons type=embedded debug=f platform=$VARIANT -c
fi

# build LightningCore Base onto nfsroot image
scons type=embedded debug=f platform=$VARIANT

# package up all Base components on nfsroot image
./CoreTools/CreatePackages.py -r $ROOTFS_PATH -o $ROOTFS_PATH

popd
exit 0
