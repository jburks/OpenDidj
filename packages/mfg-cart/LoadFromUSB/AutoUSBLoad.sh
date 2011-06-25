#!/bin/sh

#set -x

#####################################################################
#    AutoUSBLoad is a program than will allow an operator to copy 
#	Didj content to the /Didj USB drive using a host PC rather than
#	an ATAP card.  The unit must have the appropriate Firmware 
#	preloaded so it can mount the USB drives properly.
#
#     Written by Nathan Durrin
#
#   Function:  This file will load the base content packages from the PC to the  
#              base unit and validate the files unpacked OK.
#
#   Usage:  Copy the content packages into the Packages folder in the same 
#		directory as this script
#	    Script must be run as sudo for unmounting operations to work
#	    First time run, follow the prompts to map the USB ports
#	        Each subsequent run will not require this step, BUT
#		if you change hardware, you must remap the ports.
#
#   Revision Table..................................................
#   Date	Rev   Author		Change	 
#   ----------------------------------------------------------------
#   12/06/07    001    N.Durrin	        Initial release 
#   04/28/08	002    N.Durrin		Changed to seperate files for each port due to bug on cleared screen
#					when multiple units are plugged in at the same time.
#   05/01/08    003    N.Durrin		Added unmounting MFG_PAYLOAD and Cartridge if found during 
#					port mapping in case user is using an ATAP for this step.
#					Added additional options while starting AutoUSBLoad.  options are:
#						-a   	    allows you to add ports to current map
#						-d [PORT]   allows you to remove a port from current map
#						-p          forces creating a new port map"
#					Fixes status getting lost bug where would get stuck on LOADING...
#
######################################################################



VERSION="003"


rm -rf tmp

mkdir -p tmp

#function for mapping the ports which will be used for updating Didj Units
map_ports() 
{
	if [ "$1" = "new" ] ; then 
		#echo "new"
		rm -f port_map.txt
		touch port_map.txt
	elif [ "$1" = "delete" ] ; then 
		#echo "delete $2"
		for port in `cat port_map.txt | tr -t " " "*"` ; do 
			if [ "`echo $port | grep -c $2`" = "0" ] ; then 
				echo $port | tr -s "*" " " > tmp/port_map_deleted.txt
			fi
		done
		echo "$2 was deleted from port map"
		mv tmp/port_map_deleted.txt port_map.txt
		exit 0
	elif [ "$1" = "add" ] ; then 
		#echo "add"
		if [ ! -e port_map.txt ] ; then
			echo "No port map exists, creating a new one"
			touch port_map.txt
		fi	
	fi

	DONE=0
	PORT_FILE=1
	# get board info
	echo "This is the Port Map Utility.  It will show you which USB port"
	echo "is which so you can lable your ports and know which unit the "
	echo "update program is referencing when providing status."
	echo " "
	echo "Using a single Didj, plug it into each USB port in the order you"
	echo "choose following the prompts.  After each port is found, you will"
	echo "have the chance to label it with your own name so you can physically"
	echo "label the port for easy reference."
	echo " "
	touch tmp/mounts.log	
	#echo "Ensure you have NO Didj's plugged in right now."
	#while read -p "Press <Enter> when ready.... " ans; do  
	#	mount | grep /media/Didj | cut -d " " -f3 > tmp/mounts.log
	#	break;
	#done
	echo "Plug the Didj into the First USB port........."
	while [ $DONE = 0 ] ; do 
		mount | grep /media/Didj | cut -d " " -f3 > tmp/new_mounts.log
		if [ "`diff tmp/new_mounts.log tmp/mounts.log | grep -c \<`" -ne "0" ] ; then 
			for usb in `diff tmp/new_mounts.log tmp/mounts.log | grep \< | cut -d " " -f2` ; do
				DEVICE="`mount | grep "$usb " |  cut -d / -f3 | cut -d " " -f1`"
				PORT="`ls -l /dev/disk/by-path/ | grep "$DEVICE$" | cut -d " " -f9 | cut -d - -f4`"
				echo "$usb found on port $PORT"
				sudo umount -f $usb  	
				if [ "`cat port_map.txt | grep -c $PORT`" = "0" ] ; then
					while read -p "Enter a label for this port (optional) " ans; do
						LABEL=$ans
						break;
					done
					echo "$LABEL ($PORT)" >> port_map.txt
				else
					echo "Port $PORT already exists in current port map"
				fi

				if [ ! "`mount | grep -c MFG_PAYLOAD`" = "0" ] ; then 
					sudo umount -f /media/MFG_PAYLOAD
				fi
				if [ ! "`mount | grep -c Cartridge`" = "0" ] ; then 
					sudo umount -f /media/Cartridge
				fi
			done
			cp -f tmp/new_mounts.log tmp/mounts.log

			while read -p "Do you want to map another port? (y/n) " ans; do
				if [ "$ans" = "y" ]; then
					#sudo umount -f $usb
					echo "Unplug the unit and plug into next port"
					DONE=0
					break;
				elif [ "$ans" = "Y" ]; then
					#sudo umount -f $usb
					echo "Unplug the unit and plug into next port"
					DONE=0
					break;
				elif [ "$ans" = "n" ]; then
					DONE=1
					echo "Unplug all units at this time"
					while read -p "Press <Enter> when all units are UNPLUGGED.... " ans; do
						break;
					done
					break;
				elif [ "$ans" = "N" ]; then
					DONE=1
					echo "Unplug all units at this time"
					while read -p "Press <Enter> when all units are UNPLUGGED.... " ans; do
						break;
					done
					break;
				fi
			done
		else
			cp -f tmp/new_mounts.log tmp/mounts.log
		fi
	done
	return 0	
}


refresh_screen () {
	#refresh status screen display
	clear
	echo "LoadFromUSB version $VERSION"
	echo " " 
	LINE_NUM=1
	NUMBER_OF_PORTS_MAPPED="`cat port_map.txt | grep -c \(*\)`"
	while [ $LINE_NUM -le $NUMBER_OF_PORTS_MAPPED ] ; do 
		LINE="`cat tmp/port$LINE_NUM.txt`"
		STATUS=""
		if [ -e tmp/$LINE_NUM.stat ] ; then 
			STATUS="`cat tmp/$LINE_NUM.stat`"
		fi

		if [ "`echo $STATUS | grep -c FAILED`" -ge "1" ] ; then 		
			echo "$LINE \033[1;37;41m===== FAILED =====\033[0m"
			#echo "   `cat tmp/$LINE_NUM.stat | grep ERROR`"
		elif [ "`echo $STATUS | grep -c PASSED`" = "1" ] ; then
			echo "$LINE ..... \033[1;37;42m   PASSED   \033[0m"
		elif [ "`echo $STATUS | grep -c LOADING`" = "1" ] ; then
			echo "$STATUS." > tmp/$LINE_NUM.stat
			echo "$LINE $STATUS"
		elif [ "`echo $STATUS | grep -c TIMED`" = "1" ] ; then
			echo "$LINE ..... \033[1;37;41m $STATUS \033[0m"
		else 
			echo "$LINE"
		fi 
		echo " "
		LINE_NUM=`expr $LINE_NUM + 1`
		date +%s > tmp/time_stamp
	done
	return 0
} 

clear
OUT_MESSAGE=""
PM=0

DEL_PORT=""
while getopts "pad:" opt; do
	case "$opt" in
		"p")
			PM="new"
			;;
		"a")
			PM="add"
			;;
		"d")
			PM="delete"
			DEL_PORT="$OPTARG"
			;;
		"?")
			echo "Usage: AutoUSBLoad [-option]"
			echo " "
			echo " Options include:"
			echo "     -a          allows you to add ports to current map"
			echo "     -d [PORT]   allows you to remove a port from current map"
			echo "     -p          forces creating a new port map"
			echo " " 			
			exit 1
			;;
		*)
			exit 1
			;;
	esac
done

if [ "`ls Packages | grep -c .lfp`" = "0" ] ; then 
	echo "ERROR: No packages to load on the units in Packages directory"
	echo " "
	echo "Please put the packages you want to load over USB in the Packages directory"
	echo " "
	exit 1
fi


if [ ! -e port_map.txt ] ; then 
	if [ $PM = 0 ] ; then 
		echo "No port map found."
	fi
	PM="new"
fi

if [ ! "$PM" = "0" ] ; then
	map_ports $PM $DEL_PORT
fi

#BUILD PORT AND STAT FILES FROM port_map.txt
STAT_NUM="1"
for port in `cat port_map.txt | tr -t " " "*"` ; do 
	echo $port | tr -s "*" " " > tmp/port$STAT_NUM.txt
	touch tmp/$STAT_NUM.stat
	STAT_NUM=`expr $STAT_NUM + 1`
done

#ls -l /dev/disk/by-id | Didj > tmp/mounts.log # BY ID
#touch tmp/mounts.log
ls -al /dev/disk/by-path/ | grep usb- | tr -s " " | cut -d " " -f8- | cut -d- -f4 > tmp/mounts.log # BY PATH
ls tmp | grep .mtd >> tmp/mounts.log
###mount | grep /media/Didj | cut -d " " -f1 > tmp/mounts.log
##cp -f port_map.txt tmp/port_map_new.txt
##cp -f tmp/port_map_new.txt port_map_status.txt
clear 
#cat port_map_status.txt
refresh_screen

while [ 1=1 ] ; do 
	#ls -l /dev/disk/by-id | Didj > tmp/new_mounts.log # BY ID
	ls -al /dev/disk/by-path/ | grep usb- | tr -s " " | cut -d " " -f8- | cut -d- -f4 > tmp/new_mounts.log # BY PATH
	ls tmp | grep .mtd >> tmp/new_mounts.log	
	###mount | grep /media/Didj | cut -d " " -f1> tmp/new_mounts.log
	diff -q tmp/new_mounts.log tmp/mounts.log > /dev/null
	DIFFER="`echo $?`"

	if [ ! "$DIFFER" = "0" ] ; then 
		
			
		if [ "`diff tmp/new_mounts.log tmp/mounts.log | grep -c \<`" -eq "0" ] ; then 
			#Didj was unmounted, check it's status
			for LOG_IN in `ls tmp | grep .pid | cut -d. -f1` ; do	
				if [ ! -e tmp/$LOG_IN.mtd ] ; then
					#Drive was unmounted
					rm -f tmp/$LOG_IN.pid
				fi
			done
			cp -f tmp/new_mounts.log tmp/mounts.log	

		else 
			#New Didj Found
			for PORT in `diff tmp/new_mounts.log tmp/mounts.log | grep \< | cut -d " " -f2` ; do
				###DEVICE="`mount | grep "$usb1 " |  cut -d / -f3 | cut -d " " -f1`"
				DEVICE="`ls -l /dev/disk/by-path | grep "$PORT" |  cut -d / -f3`"	# | cut -d " " -f1`"  
				if [ "`ls -l /dev/disk/by-id | grep "../../$DEVICE" | grep -c Didj`" -eq "1" ] ; then
					# NEW PORT FOUND THAT WAS NOT PREVIOUSLY MAPPED
					if [ "`cat port_map.txt | grep -c $PORT`" -eq "0" ] ; then 
						echo " ($PORT)" >> port_map.txt
						NUMBER_OF_PORT="`cat port_map.txt | grep -c \(*\)`"
						echo " ($PORT)" > tmp/port$NUMBER_OF_PORT.txt
						touch tmp/$NUMBER_OF_PORT.stat 
					fi
							
					usb1=""
					while [ "X$usb1" = "X" -o "`mount | grep -c "/dev/$DEVICE on"`" -eq "0" ] ; do
						usb1="`mount | grep "/dev/$DEVICE on" | cut -d" " -f3`"
						TIME_STAMP=`date +%s` 
						OLD_TIME=`cat tmp/time_stamp`
						if [ "`expr $TIME_STAMP - $OLD_TIME`" -ge "8" ] ; then 
							break
						fi
					done
					if [ "X$usb1" != "X" ] ; then
						sudo ./copy_usb.sh -d $usb1  &
						JOB_PID=$!
						echo "$JOB_PID.mtd" >> tmp/new_mounts.log	
						STAT_FILE="`cat port_map.txt | grep -n $PORT | cut -d: -f1`"
						echo "$STAT_FILE.stat" > tmp/$JOB_PID.pid
						touch tmp/$JOB_PID.mtd	
						echo "LOADING" > tmp/$STAT_FILE.stat
						echo $usb1 >> tmp/$JOB_PID.pid
						echo $PORT >> tmp/$JOB_PID.pid
						echo $DEVICE >> tmp/$JOB_PID.pid
					else 	
						STAT_FILE="`cat port_map.txt | grep -n $PORT | cut -d: -f1`"
						echo "USB CONNECT TIMED OUT" > tmp/$STAT_FILE.stat
					fi					
				fi
			done
			cp -f tmp/new_mounts.log tmp/mounts.log
		fi


		refresh_screen


		#cat port_map_status.txt
	fi



	




	for PORT_IN in `cat port_map.txt | tr -d " " | cut -d "(" -f2 | cut -d ")" -f1` ; do
		STAT_FILE="`cat port_map.txt | grep -n $PORT_IN | cut -d: -f1`"
		if [ -e tmp/comp_$STAT_FILE.stat ] ; then
			mv tmp/comp_$STAT_FILE.stat tmp/$STAT_FILE.stat
		fi 
		STAT_STATUS="`cat tmp/$STAT_FILE.stat`"
		if [ "X$STAT_STATUS" != "X" -a "`ls -l /dev/disk/by-path/ | grep -c "usb-$PORT_IN"`" -eq "0" ] ; then
			echo "" > tmp/$STAT_FILE.stat
			refresh_screen		
		fi
	done

	TIME_STAMP=`date +%s` 
	OLD_TIME=`cat tmp/time_stamp`
	
	if [ "`expr $TIME_STAMP - $OLD_TIME`" -ge "4" ] ; then 
		refresh_screen
	fi

done





