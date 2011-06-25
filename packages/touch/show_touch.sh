#!/bin/sh
# read touchscreen values

# set GPIO for X measurement                                                    
gpio-control /dev/gpio func      1 2 0                                          
gpio-control /dev/gpio outenable 1 2 1                                          
gpio-control /dev/gpio outvalue  1 2 1                                          
gpio-control /dev/gpio func      1 3 0                                          
gpio-control /dev/gpio func      1 4 0                                          
gpio-control /dev/gpio outenable 1 4 1                                          
gpio-control /dev/gpio outvalue  1 4 0                                          
gpio-control /dev/gpio func      1 5 0                                          
                                                                                
x0=`cat /sys/devices/platform/lf1000-adc/channel0`                              
x1=`cat /sys/devices/platform/lf1000-adc/channel1`                              
x2=`cat /sys/devices/platform/lf1000-adc/channel2`                              
x3=`cat /sys/devices/platform/lf1000-adc/channel3`                              
                                                                                
# set GPIO for Y measurement                                                    
gpio-control /dev/gpio func      1 2 0                                          
gpio-control /dev/gpio func      1 3 0                                          
gpio-control /dev/gpio outenable 1 3 1                                          
gpio-control /dev/gpio outvalue  1 3 1                                          
gpio-control /dev/gpio func      1 4 0                                          
gpio-control /dev/gpio func      1 5 0                                          
gpio-control /dev/gpio outenable 1 5 1                                          
gpio-control /dev/gpio outvalue  1 5 0                                          
                                                                                
y0=`cat /sys/devices/platform/lf1000-adc/channel0`                              
y1=`cat /sys/devices/platform/lf1000-adc/channel1`                              
y2=`cat /sys/devices/platform/lf1000-adc/channel2`                              
y3=`cat /sys/devices/platform/lf1000-adc/channel3`

echo "X0:$x0 X1:$x1 X2:$x2 X3:$x3  Y0:$y0 Y1:$y1 Y2:$y2 Y3:$y3"
