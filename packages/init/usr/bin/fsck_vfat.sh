#!/bin/sh
#
# fsck_vfat.sh
#   $1 -- device to check
#   $2 -- log file name
#
# Run fsck.vfat in script so that errorlevel result can be written to log file.
#
FSCK="/sbin/fsck.vfat -n"
DEVICE=$1
LOG_FILE=$2

$FSCK $DEVICE > $LOG_FILE 2>&1
echo "EXIT=$?" >> $LOG_FILE
