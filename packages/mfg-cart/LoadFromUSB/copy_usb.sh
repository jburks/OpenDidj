#!/bin/sh

#set -x 
#echo `date`
DIR_PATH="Packages"
LFPKG_PATH="./"

THREAD_PID="`echo $$`"

LOG_FILE="tmp/$THREAD_PID.pid"

while getopts "d:" opt; do
	case "$opt" in
		"d")
			DIR=$OPTARG
			;;
		"?")
			exit 1
			;;
		*)
			exit 1
			;;
	esac
done

rm -rf $DIR/*
#sleep 1
STAT_FILE=""
while [ ! "`echo $STAT_FILE | grep -c stat`" = "1" ] ; do
	STAT_FILE="`cat $LOG_FILE | grep .stat`"
done

for LFP in `find $DIR_PATH/ -name "*.lfp"` ; do
	CUR_LFP="$LFP"
	BASE="$DIR"
	if [ `echo $LFP | grep -c bootstrap` = "1" -o  `echo $LFP | grep -c firmware` = "1" ] ; then 
		cp -f $LFP $DIR 	# this will result in firmware and/or bootloader update
	else
		$LFPKG_PATH/lfpkg -a install -b $BASE \ $LFP >/dev/null
		if [ ! "`echo $?`" = "0" ] ; then 
			#echo "ERROR: $CUR_LFP failed installation on $DIR" > tmp/$STAT_FILE
			sudo umount -f $DIR 
			#echo "status=FAILED" > tmp/$STAT_FILE
			echo "status=FAILED" > tmp/comp_$STAT_FILE
			rm -f tmp/$THREAD_PID.mtd
			exit 1   
		fi
		# This is to immediately validate	
		#echo "Validating $CUR_LFP version $META_VERSION" 			
		$LFPKG_PATH/lfpkg -a validate -b $BASE \ $LFP >/dev/null 2>/dev/null 
		if [ ! "`echo $?`" = "0" ] ; then 
			#echo "ERROR: $CUR_LFP failed validation on $DIR" > tmp/$STAT_FILE
			sudo umount -f $DIR 
			#echo "status=FAILED" > tmp/$STAT_FILE
			echo "status=FAILED" > tmp/comp_$STAT_FILE
			rm -f tmp/$THREAD_PID.mtd
			exit 1   
		fi
	fi
done
sudo umount -f $DIR    
#echo "status=PASSED" > tmp/$STAT_FILE
echo "status=PASSED" > tmp/comp_$STAT_FILE
rm -f tmp/$THREAD_PID.mtd
exit 0

