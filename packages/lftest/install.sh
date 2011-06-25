#!/bin/bash

set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*

pushd $PROJECT_PATH/packages/lftest

# invoke make and make install in each directory.  Also install the lftest_xxx
# scripts.
mkdir -p $ROOTFS_PATH/var/lib/lftest/
for t in `ls`; do
	if [ ! -d $t ]; then
		continue
	fi

	echo "Installing test $t"

	if [ ! -e $t/install.sh ]; then
		echo "test $t lacks an install script!"
		exit 1
	fi

	$t/install.sh
done

# install the master script and functions script
cp runall $ROOTFS_PATH/usr/bin/
cp functions $ROOTFS_PATH/var/lib/lftest/

popd

exit 0
