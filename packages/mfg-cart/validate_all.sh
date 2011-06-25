#!/bin/sh

set_screen () {
    SCREEN_NAME=$1

    layer-control /dev/layer0 s enable off
    layer-control /dev/layer0 s format B8G8R8
    layer-control /dev/layer0 s hstride 3
    layer-control /dev/layer0 s vstride 960
    layer-control /dev/layer0 s position 0 0 320 240
    layer-control /dev/layer0 s dirty
    imager /dev/layer0 /var/screens/$SCREEN_NAME
    layer-control /dev/layer0 s enable on
    layer-control /dev/layer0 s dirty
        
    return 0
}

log_text () {
    drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
    drawtext /dev/layer0 /test/monotext8x16.rgb "$1"	

    echo $1
    if [ -n "$2" ] ; then
        if [ -f $2 ] ; then
            echo $1 >> $2
        fi
    fi

    return 0
}

#set_screen "Atap_splash.png"

if [ "0" = `grep -c prg_LF1000_uniboot /proc/mtd` ] ; then
    log_text "Target partitions not present. Aborting ..." $LOGFILE
    exit 1
fi

# Setup device names for MTD partitions

bootpart="/dev/`grep prg_LF1000_uniboot /proc/mtd | cut -d: -f1`"
kernel0part="/dev/mtd`grep prg_Kernel0 /proc/mtd | cut -d: -f1 | cut -c 4-`"
kernel1part="/dev/mtd`grep prg_Kernel1 /proc/mtd | cut -d: -f1 | cut -c 4-`"
root0part="/dev/mtd`grep prg_Linux_RFS0 /proc/mtd | cut -d: -f1 | cut -c 4-`"
root1part="/dev/mtd`grep prg_Linux_RFS1 /proc/mtd | cut -d: -f1 | cut -c 4-`"
mfgmnt="/dev/mtdblock`grep prg_Manufacturing_Data /proc/mtd | cut -d: -f1 | cut -c 4-`"
extsize="`grep EXT /proc/mtd | cut -d " " -f2`"

if [ "$extsize" != "00000000" ] ; then
    mount -t jffs2 $mfgmnt /mnt
    LOGFILE="/mnt/validate.log"
    PACKAGES=`find /opt/Didj -name meta.inf -exec dirname {} \;`
    log_text "Validate all, run from a manufacturing cartridge." $LOGFILE
    MAKE_MFG_CART=1
else
    LOGFILE="/mfgdata/validate.log"
    PACKAGES=`find /Didj -name meta.inf -exec dirname {} \;`
    log_text "Validate all, run from a base unit.\nOnly limited validation can be performed." $LOGFILE
    MAKE_MFG_CART=0
fi

echo "" > $LOGFILE

log_text "Validating installed software" $LOGFILE

if [ -z "$BOOTSTRAP_IMAGE_PATH" ] ; then 
    if [ "$MAKE_MFG_CART" -eq "2" ] ; then
        BOOTSTRAP_IMAGE_PATH=/Didj/Base/bootstrap-ME_LF1000
    else
        BOOTSTRAP_IMAGE_PATH=/Didj/Base/bootstrap-LF_LF1000
    fi    
fi    
if [ -z "$FIRMWARE_IMAGE_PATH" ] ; then 
    if [ "$MAKE_MFG_CART" -eq "2" ] ; then
        FIRMWARE_IMAGE_PATH=/Didj/Base/firmware-ME_LF1000
    else
        FIRMWARE_IMAGE_PATH=/Didj/Base/firmware-LF_LF1000
    fi
fi

LDR_FILE="lightning-boot.bin"
LDR_SUM="`cat $BOOTSTRAP_IMAGE_PATH/lightning-boot.md5`"
if [ -e $FIRMWARE_IMAGE_PATH/kernel.bin ] ; then
    KERNEL_FILE="kernel.bin"
else
    KERNEL_FILE="kernel.jffs2"
fi
KERNEL_SUM="`cat $FIRMWARE_IMAGE_PATH/kernel.md5`"
ROOTFS_FILE="erootfs.jffs2"
ROOTFS_SUM="`cat $FIRMWARE_IMAGE_PATH/erootfs.md5`"

BOOTLDR=$BOOTSTRAP_IMAGE_PATH/$LDR_FILE
KERNEL=$FIRMWARE_IMAGE_PATH/$KERNEL_FILE
ROOTFS=$FIRMWARE_IMAGE_PATH/$ROOTFS_FILE
BOOTVER=`lfpkg -a version $BOOTSTRAP_IMAGE_PATH`
FIRMVER=`lfpkg -a version $FIRMWARE_IMAGE_PATH`

echo "BOOTLDR=$BOOTLDR"
echo "KERNEL=$KERNEL"
echo "ROOTFS=$ROOTFS"

# Get image file sizes

if [ -e $BOOTLDR ] ; then 
    bootsize=`ls -l $BOOTLDR | tr -s "" " " | cut -d" " -f5`
fi
if [ -e $KERNEL ] ; then 
    kernelsize=`ls -l $KERNEL | tr -s "" " " | cut -d" " -f5`
fi
if [ -e $ROOTFS ] ; then 
    rootfsize=`ls -l $ROOTFS | tr -s "" " " | cut -d" " -f5`
fi

# OK, everything is prepped and ready. Start the validation.

ERRORS_FOUND=0

if [ -e $BOOTLDR ] ; then 
    if [ "$LDR_SUM" = "`nanddump -m -l $bootsize $bootpart 2> /dev/null`" ] ; then
        log_text "Boot sector version $BOOTVER correctly programmed." $LOGFILE
    else
        log_text "Boot sector version $BOOTVER checksum mismatch" $LOGFILE
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
    fi
else
    log_text "Boot sector image file not found" $LOGFILE
    ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
fi

if [ -e $KERNEL ] ; then 
    if [ "$KERNEL_SUM" = "`nanddump -m -l $kernelsize $kernel0part 2> /dev/null`" ] ; then
        log_text "Linux kernel partition 0 correctly programmed" $LOGFILE
    else
        log_text "Linux Kernel version $FIRMVER partition 0 checksum mismatch" $LOGFILE
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
    fi
    if [ "$KERNEL_SUM" = "`nanddump -m -l $kernelsize $kernel1part 2> /dev/null`" ] ; then
        log_text "Linux kernel version $FIRMVER partition 1 correctly programmed" $LOGFILE
    else
        log_text "Linux Kernel version $FIRMVER partition 1 checksum mismatch" $LOGFILE
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
    fi
else
    log_text "Kernel image file not found" $LOGFILE
    ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
fi

if [ -e $ROOTFS ] ; then 
    if [  "$ROOTFS_SUM" = "`nanddump -m -l $rootfsize $root0part 2> /dev/null`" ] ; then
        log_text "Root filesystem version $FIRMVER partition 0 correctly programmed" $LOGFILE
    else
        log_text "Root filesystem version $FIRMVER partition 0 checksum mismatch" $LOGFILE
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
    fi
    if [ "$ROOTFS_SUM" = "`nanddump -m -l $rootfsize $root1part 2> /dev/null`" ] ; then
        log_text "Root filesystem version $FIRMVER partition 1 correctly programmed" $LOGFILE
    else
        log_text "Root filesystem version $FIRMVER partition 1 checksum mismatch" $LOGFILE
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
    fi
else
    log_text "Root filesystem image file not found" $LOGFILE
    ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
fi

for pkg in $PACKAGES ; do
    PKGNAME=`basename $pkg`
    INSTALLED=`lfpkg -a version $pkg`
    echo "Searching for $PKGNAME source lfp"
    SOURCE=`find /opt/mfg -name "*.lfp" | grep -i $PKGNAME`
    if [ -n "$SOURCE" ] ; then
	PAYLOAD=`lfpkg -a version $SOURCE`
        if [ "$INSTALLED" = "$PAYLOAD" ] ; then
	    log_text "Package $PKGNAME versions match: $INSTALLED" $LOGFILE
	else
	    log_text "Package $PKGNAME versions don't match: installed = $INSTALLED, payload = $PAYLOAD" $LOGFILE
            ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	fi
    else
        log_text "Source package for $PKGNAME not found. Unable to check version: installed = $INSTALLED." $LOGFILE
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
    fi
    echo "Validating $pkg..."
    lfpkg -a validate $pkg
    if [ $? -ne 0 ] ; then
        log_text "Installed package $PKGNAME version $INSTALLED failed validation" $LOGFILE
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
    else
        log_text "Installed package $PKGNAME version $INSTALLED passed validation" $LOGFILE
    fi
done

if [ "$MAKE_MFG_CART" -eq "1" ] ; then
    umount /mnt
fi

log_text "Validation complete. $ERRORS_FOUND errors found." $LOGFILE

exit $ERRORS_FOUND

