/* ili9322.h -- hardware definitions for ILI9322 LCD controller 
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 * Scott Esters <sesters@leapfrog.com>
 */

#ifndef ILI9322_H
#define ILI9322_H

#define LCD_CHIP_ID	0x96

#define LCD_GET(x)	((x | 0x8000) & 0xFFFF)
#define LCD_SET(x)	((x & ~(0x8000)) & 0xFFFF)

#define CMD_CHIPID	0x0000
#define CMD_AMPLITUDE	0x0100
#define CMD_HIGHVOLTAGE	0x0200
#define CMD_VIEWANGLE	0x0300
#define CMD_DISPLAY	0x0B00
#define CMD_CONTRAST	0x0E00
#define CMD_BRIGHTNESS	0x0F00
#define CMD_GAMMA1	0x1000
#define CMD_GAMMA2	0x1100
#define CMD_GAMMA3	0x1200
#define CMD_GAMMA4	0x1300
#define CMD_GAMMA5	0x1400
#define CMD_GAMMA6	0x1500
#define CMD_GAMMA7	0x1600
#define CMD_GAMMA8	0X1700

#endif
