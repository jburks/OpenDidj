#!/bin/sh
#
# fsck_watch.sh: Allow fsck.vfat to run for only a limited amount of time
#                or stop when multiple errors are detected.
#		 $1=device to check, $2=mount point
#

# get command line options
DEVICE=$1				# drive to check
MOUNT_POINT=$2				# mount point, needed to delete directory

# various system defines
FSCK_VFAT=/usr/bin/fsck_vfat.sh		# location of fsck_vfat script
FSCK_DELETE=/usr/bin/fsck_delete.sh	# location of fsck_delete script
LOGFILE=/tmp/fsck.log			# log file location
MAX_SECONDS=30				# run for 30 seconds max
MAX_LOGLINES=20				# max number of log file lines

# exit codes
exit_code=NONE				# our exit code, not set
SUCCESS=0				# no error, matches fsck.vfat return
ERROR_DETECTED=1			# errors detected, matches fsck.vfat return
ERROR_USAGE=2				# usage error, matches fsck.vfat return
ERROR_TIMEOUT=3				# timeout error, fsck.vfat process killed
ERROR_LOGFILE=4				# logfile too large


errExit() {
  child_pid=$1
  echo "errExit killing $1"
  kill -s SIGINT $1
  exit -1
}

touch $LOGFILE				# clean out old file then start fsck check
$FSCK_VFAT $DEVICE $LOGFILE & fsck_vfat_pid=$!

trap 'errExit $fsck_vfat_pid' SIGINT SIGTERM

sleep_count=$MAX_SECONDS
looping=1

# loop until log file too large, process quits, or timeout expires
while [ "$looping" -eq "1" ]; do
  if [ `wc $LOGFILE | awk '{print $1}'` -gt $MAX_LOGLINES ]
  then
    # logfile too large, kill process, then exit loop
    kill -s TERM $fsck_vfat_pid
    wait $fsck_vfat_pid
    echo "ERROR:logfile too large, killed $FSCK_VFAT" >> $LOGFILE
    looping=0
    exit_code=$ERROR_LOGFILE
  else
    if [ `ps -o pid | grep $fsck_vfat_pid` ]
    then
      # process running, sleep
      sleep 1
      let "sleep_count -= 1"
      if [ $sleep_count -le "0" ]
      then
        # timeout expired, kill process, then exit loop
        kill -s TERM $fsck_vfat_pid
        wait $fsck_vfat_pid
        echo "ERROR: timeout expired, killed $FSCK_VFAT" >> $LOGFILE
        looping=0
        exit_code=$ERROR_TIMEOUT
      fi
    else
      # process finished, exit loop
      wait $fsck_vfat_pid
      looping=0
      if [ `tail -1 $LOGFILE | awk 'BEGIN{FS="="}{print $1}'` == "EXIT" ]
      then
        exit_code=`tail -1 $LOGFILE | awk 'BEGIN{FS="="} {print $2}'`
      else
        exit_code=$ERROR_DETECTED
      fi
    fi
  fi
done

# remove traps
trap '' SIGINT
trap '' SIGTERM

echo "exit_code="$exit_code

# if errors detected, try to remove bad data file
if [ $exit_code == $ERROR_DETECTED ]
then
  # $FSCK_DELETE $LOGFILE $DEVICE $MOUNT_POINT
  # exit_code=$?
  echo "skipping fsck delete option on errors"
fi

exit $exit_code 
