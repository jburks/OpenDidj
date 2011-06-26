#!/bin/bash

# assume all variables are set.  Create directory stucture under ROOTFS_PATH
set -e

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# parse args
set_standard_opts $*
mkdir -p $TFTP_PATH
mkdir -p $ROOTFS_PATH
pushd $ROOTFS_PATH
mkdir -p bin dev etc lib proc sbin tmp usr var sys boot mnt mnt2 opt Didj flags Cart mfgdata
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p opt/brio opt/apps opt/cart opt/Didj opt/mfg opt/prg_mfg
mkdir -p mnt2/mfg mnt2/flags
chmod 777 Didj opt/Didj opt/mfg opt/prg_mfg Cart

if [ $EMBEDDED -eq 0 ]; then
	mkdir -p usr/include
	mkdir -p usr/local/include
fi

mkdir -p var/lib var/lock var/log var/run var/tmp
chmod 1777 var/tmp
chmod a+rwx $ROOTFS_PATH
popd

exit 0
