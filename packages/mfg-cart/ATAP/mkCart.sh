#!/bin/sh

set -e
# kill any application running.
MAIN_PID=""
if [ -e /tmp/main_app_pid ]; then
	MAIN_PID=`cat /tmp/main_app_pid`
	kill -9 $MAIN_PID
fi

# disable the USB
# usbctl -d mass_storage -a disable

touch /flags/main_app
touch /flags/mfcart
if [ -e /opt/mfg/S52flash ] ; then 
	echo "Updating the S52flash script."
	mv /opt/mfg/S52flash /etc/init.d/flash
	cp -s /etc/init.d/flash /etc/rc.d/S52flash
fi

if [ -e /opt/mfg/S53status ] ; then 
	echo "Updating the S52status script."
	mv /opt/mfg/S53status /etc/init.d/status
	cp -s /etc/init.d/status /etc/rc.d/S53status
fi

if [ "`ls /opt/mfg | grep -c .rgb`" -ge "1" ] ; then 
	echo "Copying status rgb screens."
	mv /opt/mfg/*.rgb /var/screens
fi

if [ "`ls /opt/mfg | grep -c .png`" -ge "1" ] ; then 
	echo "Copying status png screens."
	mv /opt/mfg/*.png /var/screens
fi

if [ -e /var/screens/bootsplash.rgb ] ; then 
    echo "Updating the bootsplash screen."
    mount -t jffs2 /dev/mtdblock3 /mnt2
    mv /var/screens/bootsplash.rgb /mnt2
    umount mnt2
fi 

if [ -e /opt/mfg/lfpkg ] ; then 
	echo "Updating the lfpkg file."
	mv /opt/mfg/lfpkg /usr/bin
fi
if [ -e /opt/mfg/mkbase.sh ] ; then 
	echo "Updating the mkbase.sh file"
	mv /opt/mfg/mkbase.sh /usr/bin
fi

for LFP in `find /opt/mfg/FW_packages/ -name "*.lfp"` ; do
			
	CUR_LFP="`echo $LFP | cut -d/  -f5 | cut -d- -f1 `"
	CUR_BRD="`echo $LFP | cut -d/  -f5 | cut -d- -f2 `"
	BASE="/Didj"
	DIR=""
	ACTION=""
	PKG="`echo $LFP `"
	META_VERSION=" `echo $LFP | cut -d- -f3 | tr -d lfp ` "


	echo "Latest $CUR_LFP-$CUR_BRD version is $META_VERSION"	

	if [ -e /Didj/Base/$CUR_LFP-$CUR_BRD/meta.inf ] ; then		
		CUR_VER="`cat /Didj/Base/$CUR_LFP-$CUR_BRD/meta.inf | grep '\<Version' | cut -d\\" -f2`" 
		echo "Your current ATAP's $CUR_LFP-$CUR_BRD verion is $CUR_VER."
		if [ "`echo $CUR_VER | tr -d .- `" = "`echo $META_VERSION | tr -d .- `" ] ; then 
			echo "Your ATAP's $CUR_LFP-$CUR_BRD payload is up to date"
		else 
			rm -Rf /Didj/Base/$CUR_LFP-$CUR_BRD*

			echo "Updating ATAP's $CUR_LFP-$CUR_BRD to version $META_VERSION"
			lfpkg -a install -b $BASE \ $LFP
			lfpkg -a validate -b $BASE \ $LFP
			if [ ! "`echo $?`" = "0" ] ; then 
				echo "ERROR updating to new $CUR_LFP-$CUR_BRD packages on cartridge from /opt/mfg/FW_packages"
				exit 1
			fi
		fi
	else
		echo "Installing $CUR_LFP-$CUR_BRD version $META_VERSION on ATAP cartridge"
		lfpkg -a install -b $BASE \ $LFP
		lfpkg -a validate -b $BASE \ $LFP
		if [ ! "`echo $?`" = "0" ] ; then 
			echo "ERROR updating to new $CUR_LFP-$CUR_BRD packages on cartridge from /opt/mfg/FW_packages"
			exit 1
		fi
	fi
done



exit 0





