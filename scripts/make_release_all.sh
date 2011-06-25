#!/bin/bash
# Wrapper for make_release.sh: Create all releases.  Put it in RELEASE_PATH

set -e

# move to the scripts directory
cd `dirname $0`

. $PROJECT_PATH/scripts/functions

# make sure all of the environment variables are good
check_vars

# exit if the user is root
check_user

# Form Factor Board
TARGET_MACH=LF_LF1000 ./make_release.sh -c

# Form Factor Board with MLC NAND
#TARGET_MACH=LF_MLC_LF1000 ./make_release.sh -c

# Development Board (with u-boot added)
TARGET_MACH=ME_LF1000 ./make_release.sh -u -c 

exit 0
