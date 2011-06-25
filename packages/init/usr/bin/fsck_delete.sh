#!/bin/sh
#
# fsck_delete.sh: delete files listed in fsck.log file
# $1=log file, $2=block device, $3=mount point

FSCK_LOGFILE=$1
DEVICE=$2
MOUNT_POINT=$3

FSCK_VFAT=/sbin/fsck.vfat

MISSING_CLUSTER=0	# need to reclaim a missing cluster

# return codes, matches fsck.vfat
SUCCESS=0
ERROR_DETECTED=1

# show log file
echo 'vvv BEGIN '$FSCK_LOGFILE' vvv'
cat $FSCK_LOGFILE
echo '^^^ END '$FSCK_LOGFILE' ^^^'

# Cull log file, stripping out known cruft,
# leaving any unexpected strings and file names.
FILE_LIST=`\
grep -v '^ ' $FSCK_LOGFILE |\
grep -v '^/dev/mtd' |\
grep -v '^EXIT' |\
grep -v '^Leaving file system unchanged.' |\
grep -v '^dosfsck 2.11' |\
uniq`

# scan list for valid files to delete or clusters to recover
for file in $FILE_LIST; do
 if [ `echo $file | grep -v '^/Data/[0-2]/' | grep -v '^/Data/GameWide/'` ]
 then # not a file, maybe a missing cluster
   if [ ! `echo $file | grep -v '^Reclaimed '` ]
   then MISSING_CLUSTER=1	# reclaim missing cluster(s)
   else # unrecognized log file entry, do nothing
     echo "Not deleting files, unexpected entry '"$file"' found"
     exit $ERROR_DETECTED
   fi
 fi
done

# reclaim missing clusters, if any found
if [ $MISSING_CLUSTER == 1 ]
then
  $FSCK_VFAT -a $DEVICE
  if [ ! $? == 0 ]
  then exit $ERROR_DETECTED
  fi
fi

# mount device so we can delete directory
mount -t vfat -o sync,noatime $DEVICE $MOUNT_POINT
if [ ! $? == 0 ]
then exit $ERROR_DETECTED
fi

# delete listed files
for file in $FILE_LIST; do
  if [ `echo $file | grep '^/Data/GameWide'` ]
  then
    DIR=$MOUNT_POINT/Data/Gamewide
    echo 'rm -rf '$DIR
    rm -rf $DIR
  else
    DIR=$MOUNT_POINT`echo $file | awk 'BEGIN{FS="/"} {print $1$2"/"$3}'`
    echo 'rm -rf '$DIR
    rm -rf $DIR
  fi
done

# unmount device
umount $MOUNT_POINT
exit $?
