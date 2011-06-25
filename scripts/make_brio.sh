#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

# Brio is meant to be built on nfsroot image only, not erootfs image
if [ $EMBEDDED -eq 1 ]; then
	echo "Skipping Brio for $ROOTFS_PATH"
	exit 0
fi

echo "****************************************************"
echo "Building Brio onto $ROOTFS_PATH"

if [ "X$BRIO_PATH" == "X" ]; then
	BRIO_PATH=~/workspace/Brio2/
	echo "BRIO_PATH not set.  Using default $BRIO_PATH"
fi

if [ ! -d $BRIO_PATH ]; then
	echo "Error: BRIO_PATH $BRIO_PATH does not exist."
	echo "Please set BRIO_PATH environment variable."
	exit 1
fi

pushd $BRIO_PATH

if [ "$CLEAN" == "1" ]; then
	scons type=embedded -c
	scons type=xembedded -c
fi

# FIXME: use TARGET_MACH and match up with Brio's platform variants!
if [ "$TARGET_MACH" == "LF_MP2530F" ]; then
	VARIANT='Lightning_LF2530BLUE'
else
	VARIANT='Lightning_LF1000'
fi

#scons type=embedded platform_variant=$VARIANT
#scons type=xembedded platform_variant=$VARIANT
scons type=publish platform_variant=$VARIANT

# post-build install step missed by scons
cp Lightning/meta.inf $ROOTFS_PATH/Didj/Base/Brio/

popd
exit 0
