CROSS_COMPILE ?= arm-linux-
CC = $(CROSS_COMPILE)gcc
ROOTFS_PATH ?= /home/lfu/nfsroot
CFLAGS = -O

i2c-ctl: i2c-ctl.c i2c.h i2c-dev.h
	$(CC) $(CFLAGS) -o i2c-ctl i2c-ctl.c

clean:
	rm -f i2c-ctl

install: i2c-ctl i2c-cirrus-dump.sh i2c-cirrus-setup.sh
	cp i2c-ctl i2c-cirrus-dump.sh i2c-cirrus-setup.sh $(ROOTFS_PATH)/usr/bin
