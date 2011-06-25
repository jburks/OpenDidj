#! /bin/sh

# Attached is an image of the ATAP jumper settings for looping back the SD signals.
# 
# Here are the signal pairs looped in this configuration. 
# NOTE: 19 & 20 are the serial terminal signals also.
# 
# J2 Pin   Signal    GPIO Port   Jumper
# ---------------------------------------------
# 21       SD_D1     GPIOB3      J2-P21 to
# 29       SD_D2     GPIOB4      J2-P29
# 
# 22       SD_D0     GPIOB2      J5-P7 to
# 27       SD_CMD    GPIOB1      J5-P5
# 
# 28       SD_D3     GPIOB5      J5-P1 to
# 24       SD_CLK    GPIOB0      J5-P3
# 
# 19       SD_nCD    GPIOA19     J4-P3 to
# 20       SD_nWP    GPIOA20     J4-P2


usbctl -d mass_storage -a disable

# if USB is connected, skip update and allow access to USB
USB_CONNECTED=`cat /sys/devices/platform/lf1000-usbgadget/vbus`		
if [ $USB_CONNECTED = 1 ] ; then 
	echo "USB connected, aborting GPIO test"
	#usbctl -d mass_storage -a enable
	#usbctl -d mass_storage -a unlock
	# display USB Connected screen	
	echo "USB connected, aborting GPIO test" > /Didj/Log.txt		
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "USB connected, aborting GPIO test"
	usbctl -d mass_storage -a unlock
	usbctl -d mass_storage -a enable
	exit 255
fi


PASSES=10
PASS=1
ERRORS_FOUND=0
usbctl -d mass_storage -a unlock
rm -f /Didj/Log.txt
touch /Didj/Log.txt

gpio-control /dev/gpio func 0 19 0
gpio-control /dev/gpio func 0 8 0
gpio-control /dev/gpio outenable 0 19 1 > /dev/null
gpio-control /dev/gpio outenable 1 2  1 > /dev/null
gpio-control /dev/gpio outenable 1 3  1 > /dev/null
gpio-control /dev/gpio outenable 1 5  1 > /dev/null
gpio-control /dev/gpio outenable 0 8  1 > /dev/null # diable serial loopback gpio




while [ $PASS -le $PASSES ] ; do
    gpio-control /dev/gpio outvalue 0 19 1 > /dev/null
    gpio-control /dev/gpio outvalue 0 8  1 > /dev/null
    gpio-control /dev/gpio invalue  0 20   > /dev/null
    if [ $? -ne 1 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_A19 looped back to GPIO_A20: failed set"
	echo "GPIO_A19 looped back to GPIO_A20: failed set" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_A19 looped to GPIO_A20: failed set"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 254	
    fi

    gpio-control /dev/gpio outvalue 1 2  1 > /dev/null
    gpio-control /dev/gpio invalue  1 1    > /dev/null
    if [ $? -ne 1 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_B2 looped back to GPIO_B1: failed set"
	echo "GPIO_B2 looped back to GPIO_B1: failed set" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_B2 looped to GPIO_B1: failed set"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 21
    fi

    gpio-control /dev/gpio outvalue 1 3  1 > /dev/null
    gpio-control /dev/gpio invalue  1 4    > /dev/null
    if [ $? -ne 1 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_B3 looped back to GPIO_B4: failed set"
	echo "GPIO_B3 looped back to GPIO_B4: failed set" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_B3 looped to GPIO_B4: failed set"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 34
    fi

    gpio-control /dev/gpio outvalue 1 5  1 > /dev/null
    gpio-control /dev/gpio invalue  1 0    > /dev/null
    if [ $? -ne 1 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_B5 looped back to GPIO_B0: failed set"
	echo "GPIO_B5 looped back to GPIO_B0: failed set" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_B5 looped to GPIO_B0: failed set"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 50
    fi

    gpio-control /dev/gpio outvalue 0 19 0 > /dev/null
    gpio-control /dev/gpio outvalue 0 8  0 > /dev/null
    gpio-control /dev/gpio invalue  0 20   > /dev/null
    if [ $? -ne 0 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_A19 looped back to GPIO_A20: failed clear"
	echo "GPIO_A19 looped back to GPIO_A20: failed clear" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_A19 looped to GPIO_A20:failed clear"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 254
    fi

    gpio-control /dev/gpio outvalue 1 2  0 > /dev/null
    gpio-control /dev/gpio invalue  1 1    > /dev/null
    if [ $? -ne 0 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_B2 looped back to GPIO_B1: failed clear"
	echo "GPIO_B2 looped back to GPIO_B1: failed clear" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_B2 looped to GPIO_B1: failed clear"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 21
    fi

    gpio-control /dev/gpio outvalue 1 3  0 > /dev/null
    gpio-control /dev/gpio invalue  1 4    > /dev/null
    if [ $? -ne 0 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_B3 looped back to GPIO_B4: failed clear"
	echo "GPIO_B3 looped back to GPIO_B4: failed clear" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_B3 looped to GPIO_B4: failed clear"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 34
    fi

    gpio-control /dev/gpio outvalue 1 5  0 > /dev/null
    gpio-control /dev/gpio invalue  1 0    > /dev/null
    if [ $? -ne 0 ] ; then
        ERRORS_FOUND=`expr $ERRORS_FOUND + 1`
	echo "GPIO_B5 looped back to GPIO_B0: failed clear"
	echo "GPIO_B5 looped back to GPIO_B0: failed clear" >> /Didj/Log.txt
	drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
	drawtext /dev/layer0 /test/monotext8x16.rgb "GPIO_B5 looped to GPIO_B0: failed clear"
	usbctl -d mass_storage -a enable
	gpio-control /dev/gpio func 0 19 1
	exit 50
    fi

    PASS=`expr $PASS + 1`
done

echo "Validation complete. $ERRORS_FOUND errors found."
echo "Validation complete. $ERRORS_FOUND errors found." >> /Didj/Log.txt
drawtext /dev/layer0 /test/monotext8x16.rgb "                                        "	
drawtext /dev/layer0 /test/monotext8x16.rgb "Validation complete. $ERRORS_FOUND errors found."
usbctl -d mass_storage -a enable
gpio-control /dev/gpio func 0 19 1

exit $ERRORS_FOUND

