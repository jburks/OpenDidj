#!/usr/bin/env python
#
# make_map.py -- NAND Flash memory map generator.
# Andrey Yurovsky <andrey@cozybit.com>
#
# This script parses include/linux/autoconf.h and generates a suitable memory
# map for lightning_install.py based on the partition sizes it finds there and
# the 'names' table in the Settings section below.
#
# The output is sent to stdout, this script is intended to be run like:
# ./make_map.py > nandflash.map

import os
import sys
import re

############
# Settings #
############

names = ['lightning-boot.bin',	\
	 None,			\
	 None,			\
	 'kernel.bin',	\
	 'erootfs.jffs2',	\
	 'kernel.bin',	\
	 'erootfs.jffs2']

re_part = re.compile(r'#define CONFIG_NAND_LF1000_P(\d*)_SIZE 0x([0-9a-fxA-FX]*)\n')

try:
	config = os.environ['PROJECT_PATH'] + \
			'/linux-2.6.20-lf1000/include/linux/autoconf.h'
except KeyError:
	print "Error: PROJECT_PATH was not set.  Please set it."
	sys.exit(1)

###########################
# read in partition sizes #
###########################

try:
	f = open(config)
except IOError:
	print "Error: autoconf.h not found.  Have you built the kernel?"
	sys.exit(1)

parts = {}
line = f.readline()
while line:
	m = re_part.match(line)
	if m:
		parts[int(m.groups()[0])] = (m.groups())[1]
	line = f.readline()
f.close()

########################
# build the memory map #
########################

addr = 0
for i in range(0, len(names)):
	if names[i]:
		print "%s:%X" % (names[i], addr)
	addr += int(parts[i],16)
