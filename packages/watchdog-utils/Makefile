CROSS_COMPILE ?= arm-linux-
CC = $(CROSS_COMPILE)gcc
ROOTFS_PATH ?= /home/lfu/nfsroot
CFLAGS = -O -I$(KERNELDIR)/include

watchdog-ctl: watchdog-ctl.c
	$(CC) $(CFLAGS) -o watchdog-ctl watchdog-ctl.c

clean:
	rm -f watchdog-ctl

install: watchdog-ctl
	cp watchdog-ctl $(ROOTFS_PATH)/usr/bin
