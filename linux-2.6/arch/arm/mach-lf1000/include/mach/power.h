#ifndef _LF1000_POWER_H_
#define _LF1000_POWER_H_

/* default values */
#define MAX_BATTERY_MV	8000		/* max expected battery value	*/
#define LOW_BATTERY_MV	4200		/* low battery			*/
#define LOW_BATTERY_REPEAT_MV 100	/* repeat every 100mv drop	*/
/*
 * Lower critical battery level below hardware shutoff,
 * allowing play until you die.  Can still adjust level via
 * /sys/devices/platform/lf1000-power/critical_battery_mv interface
 */
#define CRITICAL_BATTERY_MV 2000	/* critical low battery		*/

/* Hysteresis low to normal Battery */
#define NORMAL_BATTERY_MV   (LOW_BATTERY_MV + 400)

enum lf1000_power_status {
	UNKNOWN 		= 0,
	EXTERNAL		= 1,	/* on external power */
	BATTERY			= 2,	/* on battery power */
	LOW_BATTERY		= 3,
	CRITICAL_BATTERY	= 4,
};
#endif
