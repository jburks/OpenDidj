#!/bin/bash
# http://matt.ucc.asn.au/dropbear/releases/dropbear-0.53.tar.gz

DB_SRC=dropbear-0.53.1.tar.gz

pushd $PROJECT_PATH/packages/dropbear

if [ ! -e $DB_SRC ]; then
	wget http://matt.ucc.asn.au/dropbear/releases/$DB_SRC
fi

DB_DIR=`echo "$DB_SRC" | cut -d '.' -f -3`

if [ "$CLEAN" == "1" ]; then
	rm -rf $DB_DIR
	tar -xzf $DB_SRC
fi

pushd $DB_DIR

./configure --prefix=$ROOTFS_PATH --host=arm-linux --disable-syslog --disable-lastlog CPPFLAGS="-I$ROOTFS_PATH/usr/include" LDFLAGS="-L$ROOTFS_PATH/usr/lib"

make PROGRAMS="dropbear dbclient dropbearkey dropbearconvert scp" MULTI=1

install -m 755 dropbearmulti $ROOTFS_PATH/bin
sudo chown root $ROOTFS_PATH/bin/dropbearmulti
sudo chgrp 0 $ROOTFS_PATH/bin/dropbearmulti
rm -f $ROOTFS_PATH/sbin/dropbear
ln -s /bin/dropbearmulti $ROOTFS_PATH/sbin/dropbear 
rm -f $ROOTFS_PATH/bin/dbclient 
ln -s dropbearmulti $ROOTFS_PATH/bin/dbclient 
rm -f $ROOTFS_PATH/bin/dropbearkey 
ln -s dropbearmulti $ROOTFS_PATH/bin/dropbearkey 
rm -f $ROOTFS_PATH/bin/dropbearconvert 
ln -s dropbearmulti $ROOTFS_PATH/bin/dropbearconvert 
rm -f $ROOTFS_PATH/bin/scp 
ln -s dropbearmulti $ROOTFS_PATH/bin/scp

if [ ! -e $ROOTFS_PATH/dev/pts ]; then
	sudo mkdir --mode=755 $ROOTFS_PATH/dev/pts
	sudo mknod $ROOTFS_PATH/dev/ptmx c 5 2
	sudo chmod 666 $ROOTFS_PATH/dev/ptmx
fi

if [ ! -e $ROOTFS_PATH/root ]; then
	sudo mkdir $ROOTFS_PATH/root
fi

if [ ! -e $ROOTFS_PATH/etc/dropbear ]; then
	sudo mkdir $ROOTFS_PATH/etc/dropbear
fi

sudo touch $ROOTFS_PATH/etc/group
sudo echo root:x:0:0:root:/root:/bin/sh > $ROOTFS_PATH/etc/passwd
sudo echo root:\$1\$HIXo2TYM\$bdtKBhvZCfA3L2o0KTOL4\/:13881:0:99999:7::: > $ROOTFS_PATH/etc/shadow

popd

popd

exit 0
