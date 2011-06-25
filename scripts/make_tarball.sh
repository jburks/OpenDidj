#!/bin/bash
#

set -e

NAME="ME_LF1000_Linux-`date +%m%d%y-%k%M`"

echo "Making /tmp/$NAME.tar.gz ..."

pushd $PROJECT_PATH/../
echo "exporting..."
svn export ./LinuxDist/ /tmp/$NAME
pushd /tmp
echo "making archive..."
tar -czf $NAME.tar.gz ./$NAME
rm -rf ./$NAME
popd
popd
echo "/tmp/$NAME.tar.gz is ready"

exit 0
