#! /bin/sh

# set -e

# kill any application running.
MAIN_PID=""
if [ -e /tmp/main_app_pid ]; then
	MAIN_PID=`cat /tmp/main_app_pid`
	kill -9 $MAIN_PID
fi


usbctl -d mass_storage -a disable

if [ "0" = `grep -c prg_LF1000_uniboot /proc/mtd` ] ; then
   echo "Target partitions not present. Aborting ..."
   exit 1
fi

# Setup device names for MTD partitions

bootpart="/dev/`grep prg_LF1000_uniboot /proc/mtd | cut -d: -f1`"
kernel0part="/dev/mtd`grep prg_Kernel0 /proc/mtd | cut -d: -f1 | cut -c 4-`"
kernel1part="/dev/mtd`grep prg_Kernel1 /proc/mtd | cut -d: -f1 | cut -c 4-`"
root0part="/dev/mtd`grep prg_Linux_RFS0 /proc/mtd | cut -d: -f1 | cut -c 4-`"
root1part="/dev/mtd`grep prg_Linux_RFS1 /proc/mtd | cut -d: -f1 | cut -c 4-`"
root0mnt="/dev/mtdblock`grep prg_Linux_RFS0 /proc/mtd | cut -d: -f1 | cut -c 4-`"
root1mnt="/dev/mtdblock`grep prg_Linux_RFS1 /proc/mtd | cut -d: -f1 | cut -c 4-`"
mfgpart="/dev/mtd`grep prg_Manufacturing_Data /proc/mtd | cut -d: -f1 | cut -c 4-`"
mfgmnt="/dev/mtdblock`grep prg_Manufacturing_Data /proc/mtd | cut -d: -f1 | cut -c 4-`"
flagpart="/dev/mtd`grep prg_Atomic_Boot_Flags /proc/mtd | cut -d: -f1 | cut -c 4-`"
flagmnt="/dev/mtdblock`grep prg_Atomic_Boot_Flags /proc/mtd | cut -d: -f1 | cut -c 4-`"
briopart="/dev/mtd`grep prg_Brio /proc/mtd | cut -d: -f1 | cut -c 4-`"
briomnt="/dev/mtdblock`grep prg_Brio /proc/mtd | cut -d: -f1 | cut -c 4-`"
extpart="/dev/mtd`grep prg_EXT /proc/mtd | cut -d: -f1 | cut -c 4-`"
extmnt="/dev/mtdblock`grep prg_EXT /proc/mtd | cut -d: -f1 | cut -c 4-`"
extsize="`grep prg_EXT /proc/mtd | cut -d " " -f2`"

mfgdatamnt="/dev/mtdblock`grep Manufacturing_Data /proc/mtd | cut -d: -f1 | cut -c 4-`"

#echo "$bootpart is the bootstrap partition"
#echo "$kernel0part and $kernel1part are the kernel partitions"
#echo "$root0part and $root1part are the root partitions"
#echo "$mfgpart is the manufacturing partition"
#echo "$mfgmnt is the manufacturing partition block device"
#echo "$flagpart is the flags partition"
#echo "$flagmnt is the flags partition block device"
#echo "$briopart is the brio partition"
#echo "$mfgdatamnt is the base mfg partition"
#echo "$extpart is the extended partition"
#echo "$extmnt is the extended partition mount device"
#echo "$extsize is the extended partition size"


UNIT="`cat /sys/devices/platform/lf1000-nand/cartridge`"
echo "I have found a $UNIT cartridge"

if [ "$extsize" != "00000000" ] ; then
    MAKE_MFG_CART=1
    echo "Making a manufacturing cartridge" 
elif [ "$UNIT" = "development" -o "$UNIT" = "production" ] ; then
    MAKE_MFG_CART=2		# ONLY WORKS WITH ATAP CARTRIDGE WITH SW3-5 thru 8 OCOO
    # MAKE_MFG_CART=0	# uncomment this line to make it work with the cartridge that has only 1 switch
    echo "CONFIGURING YOUR LF1000 DEV BOARD"
else
    MAKE_MFG_CART=0
    echo "CONFIGURING YOUR BASE UNIT"
fi

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
FLAG_FILE="bootflags.jffs2"
if [ -e $FIRMWARE_IMAGE_PATH/kernel.bin ] ; then
    KERNEL_FILE="kernel.bin"
else
    KERNEL_FILE="kernel.jffs2"
fi
KERNEL_SUM="`cat $FIRMWARE_IMAGE_PATH/kernel.md5`"
ROOTFS_FILE="erootfs.jffs2"
ROOTFS_SUM="`cat $FIRMWARE_IMAGE_PATH/erootfs.md5`"
FLAG_SUM="`cat $BOOTSTRAP_IMAGE_PATH/packagefiles.md5 | grep bootflags | tr -s "" " " | cut -d" " -f1`"

BOOTLDR=$BOOTSTRAP_IMAGE_PATH/$LDR_FILE
FLAGS=$BOOTSTRAP_IMAGE_PATH/$FLAG_FILE
KERNEL=$FIRMWARE_IMAGE_PATH/$KERNEL_FILE
ROOTFS=$FIRMWARE_IMAGE_PATH/$ROOTFS_FILE

echo "BOOTLDR=$BOOTLDR"
echo "FLAGS=$FLAGS"
echo "KERNEL=$KERNEL"
echo "ROOTFS=$ROOTFS"



# Get image file sizes and checksums
if [ -e $BOOTLDR ] ; then 
    bootsize=`ls -l $BOOTLDR | tr -s "" " " | cut -d" " -f5`
#    bootsum=`md5sum $BOOTLDR | tr -s "" " " | cut -d" " -f1`
#    echo "bootsize = $bootsize bytes, sum = $bootsum"
fi
if [ -e $FLAGS ] ; then 
    flagssize=`ls -l $FLAGS | tr -s "" " " | cut -d" " -f5`
#    flagssum=`cat $BOOTSTRAP_IMAGE_PATH/packagefiles.md5 | grep bootflags | tr -s "" " " | cut -d" " -f1` 
#    flagssum=`md5sum $FLAGS | tr -s "" " " | cut -d" " -f1`
#    echo "flagssize = $flagssize bytes, sum = $flagssum"
fi
if [ -e $KERNEL ] ; then 
    kernelsize=`ls -l $KERNEL | tr -s "" " " | cut -d" " -f5`
#    kernelsum=`md5sum $KERNEL | tr -s "" " " | cut -d" " -f1`
#    echo "kernelsize = $kernelsize bytes, sum = $kernelsum"
fi
if [ -e $ROOTFS ] ; then 
    rootfsize=`ls -l $ROOTFS | tr -s "" " " | cut -d" " -f5`
#    rootfsum=`md5sum $ROOTFS | tr -s "" " " | cut -d" " -f1`
#    echo "rootfsize = $rootfsize bytes, sum = $rootfsum"
fi

# Mount manufacturing data partition to access serialization data
# and record this unit.
#mount -t jffs2 $mfgdatamnt /mnt2/mfg

# Reprogram boot sector

if [ -e $BOOTLDR ] ; then 
    if [ "$MAKE_MFG_CART" = "0" -a "$LDR_SUM" = "`nanddump -m -l $bootsize $bootpart 2> /dev/null`" ] ; then
        echo "Boot sector already correctly programmed"
    else
        flash_eraseall $bootpart > /dev/null
        nandwrite -p $bootpart $BOOTLDR > /dev/null 
	if [ "$LDR_SUM" = "`nanddump -m -l $bootsize $bootpart 2> /dev/null`" ] ; then
            echo "Boot sector programmed successfully"
        else
            echo "Boot sector checksum mismatch"
	    exit 1
        fi
    fi
else
    echo "Boot sector image file not found"
    exit 1
fi

# Reprogram flags partition

if [ -e $FLAGS ] ; then 
    if [ "$FLAG_SUM" = "`nanddump -m -l $flagssize $flagpart 2> /dev/null`" ] ; then
        echo "Flag partition already correctly programmed"
    else
        flash_eraseall $flagpart > /dev/null
        nandwrite -p $flagpart $FLAGS > /dev/null 
        if [ "$FLAG_SUM" = "`nanddump -m -l $flagssize $flagpart 2> /dev/null`" ] ; then
            echo "Flag partition programmed successfully"
	    umount /mnt 2> /dev/null && echo "Unmounting unexpected mounted partition"
	    mount -t jffs2 $flagmnt /mnt > /dev/null
	    if [ $? -eq 0 ] ; then
		if [ "$MAKE_MFG_CART" = "1" ]; then
    			touch /mnt/mfcart
			echo "UNLOCKED" > /mnt/usb_mass_storage
			echo "ENABLED" >> /mnt/usb_mass_storage
			echo "NOWATCHDOG" >> /mnt/usb_mass_storage
			#echo "/usr/bin/mfg-start.sh" > /mnt/main_app
		fi
		umount /mnt 2> /dev/null || echo "Flag partition failed to unmount"
	    else
		echo "Failed to create and mount boot flags filesystem."
	    fi
        else
            echo "Flag partition checksum mismatch"
	    exit 1
        fi
    fi
else
    echo "Flag partition image file not found"
    exit 1
fi

# Reprogram kernel partition

if [ -e $KERNEL ] ; then 
    if [ "$MAKE_MFG_CART" = "0" -a "$KERNEL_SUM" = "`nanddump -m -l $kernelsize $kernel0part 2> /dev/null`" ] ; then
        echo "Linux kernel partition 0 already correctly programmed"
    else
        flash_eraseall $kernel0part > /dev/null
        nandwrite -p $kernel0part $KERNEL > /dev/null 
        if [ "$KERNEL_SUM" = "`nanddump -m -l $kernelsize $kernel0part 2> /dev/null`" ] ; then
            echo "Linux Kernel partition 0 programmed successfully"
        else
            echo "Linux Kernel partition 0 checksum mismatch"
	    exit 1
        fi
    fi
    if [ "$MAKE_MFG_CART" = "0" -a "$KERNEL_SUM" = "`nanddump -m -l $kernelsize $kernel1part 2> /dev/null`" ] ; then
        echo "Linux kernel partition 1 already correctly programmed"
    else
        flash_eraseall $kernel1part > /dev/null
        nandwrite -p $kernel1part $KERNEL > /dev/null 
        if [ "$KERNEL_SUM" = "`nanddump -m -l $kernelsize $kernel1part 2> /dev/null`" ] ; then
            echo "Linux Kernel partition 1 programmed successfully"
        else
            echo "Linux Kernel partition 1 checksum mismatch"
	    exit 1
        fi
    fi
else
    echo "Kernel image file not found"
    exit 1
fi

# Try to mount the manufacturing data partition

if [ "$MAKE_MFG_CART" = "0" -a "$mfgmnt" != "/dev/mtdblock" ] ; then
    umount /mnt 2> /dev/null && echo "Unmounting unexpected mounted partition"
    mount -t jffs2 $mfgmnt /mnt > /dev/null
    if [ $? -ne 0 ] ; then
	echo "***********************************************************************************"
        echo "WARNING: mount unit's mfgdata partition failed.  I must erase partition to recover."
	echo "WARNING: Unit's Serial number will be erased!!!!!!!!!!!  You must reserialize.     "
	echo "***********************************************************************************"
        flash_eraseall $mfgpart > /dev/null
        mount -t jffs2 $mfgmnt /mnt > /dev/null
        if [ $? -ne 0 ] ; then
            echo "Failed to create and mount manufacturing data filesystem."
	    exit 1
        fi
    fi
    echo Manufacturing data filesystem mounted.
    # Create serial number file placeholder
    if [ ! -e /mnt/UnitID.txt ] ; then 
	touch /mnt/UnitID.txt	        
	#echo "0000000000000000" > /mnt/UnitID.txt
    fi
    if [ "X`cat /mnt/UnitID.txt`" = "X" ] ; then 
	echo "Unit does not have a serial number yet."
    elif [ "`cat /mnt/UnitID.txt`" = "0000000000000000" ] ; then 
	rm -f /mnt/UnitID.txt
	touch /mnt/UnitID.txt
	echo "Serial number was all 0's.  Removed invalid serial and replaced with blank serial number"
    else
	echo "Serial number is `cat /mnt/UnitID.txt`"
    fi
    umount /mnt 2> /dev/null || echo "Manufacturing partition failed to unmount"
else
    echo "Skipping manufacturing partition"
fi

# Try to mount the root partition

if [ -e $ROOTFS ] ; then 
    if [ "$MAKE_MFG_CART" = "0" -a "$ROOTFS_SUM" = "`nanddump -m -l $rootfsize $root0part 2> /dev/null`" ] ; then
        echo "Root filesystem partition 0 already correctly programmed"
    else
	mount -t jffs2 $root0mnt /mnt > /dev/null
	if [ $? -eq 0 ] ; then
     		if [ -e /mnt/.uidts ] ; then 
			mv /mnt/.uidts /flags
		fi
		umount /mnt
	fi
        flash_eraseall $root0part > /dev/null
        nandwrite -p $root0part $ROOTFS > /dev/null 
        if [ "$ROOTFS_SUM" = "`nanddump -m -l $rootfsize $root0part 2> /dev/null`" ] ; then
            echo "Root filesystem partition 0 programmed successfully"
        else
            echo "Root filesystem partition 0 checksum mismatch"
	    exit 1
        fi
    fi
    if [ "$MAKE_MFG_CART" = "0" -a "$ROOTFS_SUM" = "`nanddump -m -l $rootfsize $root1part 2> /dev/null`" ] ; then
        echo "Root filesystem partition 1 already correctly programmed"
    else
	#mount -t jffs2 $root1mnt /mnt > /dev/null
	#if [ $? -eq 0 ] ; then	
	#	if [ -e /mnt/.uidts ] ; then 
	#		mv /mnt/.uidts /flags
	#	fi
	#	umount /mnt
	#fi
        flash_eraseall $root1part > /dev/null
        nandwrite -p $root1part $ROOTFS > /dev/null 
        if [ "$ROOTFS_SUM" = "`nanddump -m -l $rootfsize $root1part 2> /dev/null`" ] ; then
            echo "Root filesystem partition 1 programmed successfully"
        else
            echo "Root filesystem partition 1 checksum mismatch"
	    exit 1
        fi
    fi
    	    
    if [ -e /flags/.uidts ] ; then 
	mount -t jffs2 $root0mnt /mnt > /dev/null
	if [ $? -eq 0 ] ; then	
		cp -f /flags/.uidts /mnt
	        umount /mnt
	else 
	   echo "Unable to mount $root0mnt."
	   exit 1
	fi
	mount -t jffs2 $root1mnt /mnt > /dev/null
	if [ $? -eq 0 ] ; then		
		cp -f /flags/.uidts /mnt
	        umount /mnt
	else 
	    echo "Unable to mount any root file systems."
	    exit 1
	fi
	rm -f /flags/.uidts
    fi
else
    echo "Root filesystem image file not found"
    exit 1
fi

echo "End of root filesystem setup"

if [ "$MAKE_MFG_CART" = "1" ] ; then
    echo "Configuring your manufacturing cartridge..."
    umount /mnt 2> /dev/null && echo "Unmounting unexpected mounted partition"
    mount -t jffs2 $flagmnt /mnt > /dev/null
    touch /mnt/mfcart
    touch /mnt/main_app
    umount /mnt 2> /dev/null || echo "Flags filesystem partition failed to unmount"

    mount -t jffs2 $root0mnt /mnt > /dev/null
	if [ ! $? -eq 0 ] ; then	
		echo "Unable to mount $root0mnt to create ATAP."
	        exit 1
	fi
	MOUNTED_RFS1="1"
    mount -t jffs2 $root1mnt /mnt2 > /dev/null   
	
	if [ ! $? -eq 0 ] ; then	
		echo "WARNOING: Unable to mount $root1mnt to create ATAP's backup RSF1."
		MOUNTED_RFS1="0"
	fi

    
    mkdir -p /mnt/opt/mfg
    if [ $MOUNTED_RFS1 -eq "1" ] ; then 
	    mkdir -p /mnt2/opt/mfg   
    fi

    if [ "1" = `mount | grep -c prg_mfg` ] ; then
        rm -rf /opt/prg_mfg/*
	rm -rf /opt/Didj/*
	if [ -d /Didj/ATAP -o -d /payload/ATAP ] ; then 	
		for path in "/payload/ATAP" "/Didj/ATAP" ;  do
            		if [ -d $path ] ; then
				rm -rf /opt/prg_mfg/*
                		echo -n "Copying payload files from $path..."
                		cp -rf $path/* /opt/prg_mfg
                		echo "done."
            		fi
		done
	elif [ -d /Didj/atap -o -d /payload/atap ] ; then
		for path in "/payload/atap" "/Didj/atap" ;  do
            		if [ -d $path ] ; then
				rm -rf /opt/prg_mfg/*
                		echo -n "Copying payload files from $path..."
                		cp -rf $path/* /opt/prg_mfg
                		echo "done."
            		fi
		done
	elif [ -d /payload ] ; then
		rm -rf /opt/prg_mfg/*		
		echo -n "Copying payload files from /payload directory..."
                cp -rf /payload/* /opt/prg_mfg
                echo "done."
        fi

  	if [ -e /opt/prg_mfg/S52flash ] ; then
	    echo "Installing cartridge startup scripts..."
	    cp /opt/prg_mfg/S52flash /mnt/etc/init.d/flash
	    cp -s /etc/init.d/flash /mnt/etc/rc.d/S52flash
	    cp /opt/prg_mfg/S53status /mnt/etc/init.d/status
	    cp -s /etc/init.d/status /mnt/etc/rc.d/S53status

    	    if [ $MOUNTED_RFS1 -eq "1" ] ; then 
	    	cp /opt/prg_mfg/S52flash /mnt2/etc/init.d/flash
	    	cp -s /etc/init.d/flash /mnt2/etc/rc.d/S52flash
	    	cp /opt/prg_mfg/S53status /mnt2/etc/init.d/status
	    	cp -s /etc/init.d/status /mnt2/etc/rc.d/S53status
    	    fi

	    echo "Installing cartridge status screens..."
	    if [ "`ls /opt/prg_mfg | grep -c .rgb`" -ge "1" ] ; then 
			echo "Copying status rgb screens."
			cp /opt/prg_mfg/*.rgb /mnt/var/screens
			if [ $MOUNTED_RFS1 -eq "1" ] ; then 
				cp /opt/prg_mfg/*.rgb /mnt2/var/screens
			fi
	    fi

	    if [ "`ls /opt/prg_mfg | grep -c .png`" -ge "1" ] ; then 
			echo "Copying status png screens."
			cp /opt/prg_mfg/*.png /mnt/var/screens
			if [ $MOUNTED_RFS1 -eq "1" ] ; then 
				cp /opt/prg_mfg/*.png /mnt2/var/screens
			fi
	    fi
    	else 
	    echo "No startup scripts.  Cartridge will be a bare boot cartridge."
    	fi

	if [ -e /opt/prg_mfg/lfpkg ] ; then 
        	echo "Updating the lfpkg file..."
		cp -f /opt/prg_mfg/lfpkg /mnt/usr/bin
		if [ $MOUNTED_RFS1 -eq "1" ] ; then 
			cp -f /opt/prg_mfg/lfpkg /mnt2/usr/bin
		fi
	fi

	if [ -e /opt/prg_mfg/bootflags ] ; then 
        	echo "Updating the bootflags file..."
		cp -f /opt/prg_mfg/bootflags /mnt/etc/init.d
		if [ $MOUNTED_RFS1 -eq "1" ] ; then 
			cp -f /opt/prg_mfg/bootflags /mnt2/etc/init.d
		fi
	fi

	if [ -e /opt/prg_mfg/mkbase.sh ] ; then 
        	echo "Updating the mkbase.sh file..."
		cp -f /opt/prg_mfg/mkbase.sh /mnt/usr/bin
		if [ $MOUNTED_RFS1 -eq "1" ] ; then 
			cp -f /opt/prg_mfg/mkbase.sh /mnt2/usr/bin
		fi
	fi
	umount /mnt 2> /dev/null || echo "Root0 filesystem partition failed to unmount"
	if [ $MOUNTED_RFS1 -eq "1" ] ; then 
		umount /mnt2 2> /dev/null || echo "Root1 filesystem partition failed to unmount"
	fi

	if [ -d /opt/prg_mfg/FW_packages ] ; then
	        for file in /opt/prg_mfg/FW_packages/*.lfp ; do
		    lfpkg -a install -b /opt/Didj $file
		    lfpkg -a validate -b /opt/Didj $file
		    if [ ! "`echo $?`" = "0" ] ; then 
			echo "ERROR $file did not pass validation"
			exit 1
		    fi
        	done
	elif [ "1" -le `ls /opt/prg_mfg | grep -c .lfp` ] ; then 
		for file in /opt/prg_mfg/*.lfp ; do
		    lfpkg -a install -b /opt/Didj $file
		    lfpkg -a validate -b /opt/Didj $file
		    if [ ! "`echo $?`" = "0" ] ; then 
			echo "ERROR $file did not pass validation"
			exit 1
		    fi
        	done	
	else 
		echo "No files found to preload the ATAP cartridge with."
	fi
    else
	    umount /mnt 2> /dev/null || echo "Root0 filesystem partition failed to unmount"
	    if [ $MOUNTED_RFS1 -eq "1" ] ; then 
	    	umount /mnt2 2> /dev/null || echo "Root1 filesystem partition failed to unmount"
	    fi
    fi
fi

echo "Programming complete"
exit 0

