#!/usr/bin/env python

import os
import sys
import struct
import getopt

TFS_MAGIC = 0x12345678
NAME_LEN = 64
BLOCK_SIZE = 0x20000

try:
	project = os.environ['PROJECT_PATH']
except:
	print "Error: PROJECT_PATH not set.  Please set it."
	sys.exit(1)

KERNEL_PATH = project+'/linux-2.6.20-lf1000/arch/arm/boot/uImage'
UBOOT_PATH = project+'/u-boot-1.1.6-lf1000/u-boot.bin'
SPLASH_PATH = project+'/packages/screens/bootsplash.rgb'

buffer = ""
summary = ""

def pack_file(path):
	global summary
	global buffer

	name = path[path.rfind('/')+1:]
	if len(name) >= NAME_LEN:
		print "error: \"%s\", name is too long" % name
		return False
	if not os.path.exists(path):
		print "error: %s not found" % path
		return False
	h = open(path)
	data = h.read()
	h.close()
	size = len(data)
	num_blocks = size / BLOCK_SIZE
	rem = BLOCK_SIZE - (size % BLOCK_SIZE)
	if rem != 0:
		num_blocks += 1
	summary += name + '\0'*(NAME_LEN-len(name)) + \
			struct.pack('l', num_blocks)
	buffer += data + '\xFF'*rem
	return True

if __name__ == '__main__':
	try:
		opts = getopt.getopt(sys.argv[1:], "ucbe")
	except getopt.GetoptError:
		pass

	add_uboot = False
	for o,a in opts[0]:
		if o == '-c':
			try:
				os.unlink('./kernel.bin')
			except OSError:
				pass
		if o == '-u':
			add_uboot = True

	# pack files
	num_files = 0
	if not pack_file(SPLASH_PATH):
		sys.exit(1)
	num_files += 1
	if not pack_file(KERNEL_PATH):
		sys.exit(1)
	num_files += 1
	if add_uboot:
		if not pack_file(UBOOT_PATH):
			sys.exit(1)
		num_files += 1

	summary = struct.pack('ll', TFS_MAGIC, num_files) + summary

	# pad summary out to erase block size
	summary += '\xFF'*(BLOCK_SIZE-len(summary))

	# write out
	h = open("kernel.bin", "w")
	h.write(summary)
	h.write(buffer)
	h.close()
