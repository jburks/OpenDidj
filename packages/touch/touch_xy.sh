#!/bin/sh
# read touchscreen values from device driver

x0=`cat /sys/devices/platform/lf1000-touchscreen/x0`                            
x1=`cat /sys/devices/platform/lf1000-touchscreen/x1`                            
x2=`cat /sys/devices/platform/lf1000-touchscreen/x2`                            
x3=`cat /sys/devices/platform/lf1000-touchscreen/x3`

y0=`cat /sys/devices/platform/lf1000-touchscreen/y0`                            
y1=`cat /sys/devices/platform/lf1000-touchscreen/y1`                            
y2=`cat /sys/devices/platform/lf1000-touchscreen/y2`                            
y3=`cat /sys/devices/platform/lf1000-touchscreen/y3`

echo "X$x0  Y:$y1"
