#!/bin/sh

VOLTAGE=/sys/devices/platform/lf1000-power/voltage
BACKLIGHT=/sys/devices/platform/lf1000-dpc/backlight
ADC=/sys/devices/platform/lf1000-adc/channel2
LOGDIR=/Didj

INCREMENT=10

# find a place to log...

for i in `seq 0 1000`; do
	LOGPATH="$LOGDIR/battery-log$i.csv"
	if [ ! -e $LOGPATH ]; then
		break
	fi
done

echo "logging to $LOGPATH"
while true; do
	echo "backlight,mv,adc" >> $LOGPATH
	for bl in `seq -128 $INCREMENT 127`; do
		echo $bl > $BACKLIGHT
		sleep 1;
		READING="`cat $VOLTAGE`"
		ADCREADING="`cat $ADC`"
		echo "$bl,$READING,$ADCREADING" >> $LOGPATH
	done
done
