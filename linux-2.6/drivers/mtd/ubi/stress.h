/*
 * drives/mtd/ubi/stress.h
 *
 * Support routines for ubi stress testing under LF1000 platform.
 *
 * Copyright 2009 LeapFrog Enterprises, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

void print_pebs(struct ubi_device *ubi, struct ubi_scan_info *si, struct list_head *which);
