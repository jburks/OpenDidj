/*
 * volumeTable.h
 *     Translate volume control to volume level
 *
 * Scott Esters
 * LeapFrog Enterprises
 *
 */

/*
 * Headphone volume translation table
 *   value=log2(index/steps + 1)*range+min
 *       where index:  array index
 *             steps:  total array entries
 *             range:  usable Cirrus Logic values
 *             min:    min Cirrus Logic setting
 *
 */

#define HEADPHONE_VOL_STEPS	(128)
#define HEADPHONE_VOL_MIN	(128)
#define HEADPHONE_VOL_MAX	(222)
#define HEADPHONE_VOL_RANGE	( HEADPHONE_VOL_MAX - HEADPHONE_VOL_MIN + 1 )

u8 headphoneVolume[128] = {
  128,129,130,131, 132,133,134,135, 136,137,138,139, 140,141,142,143, //000-015
  144,145,146,146, 147,148,149,150, 151,152,153,154, 155,155,156,157, //016-031
  158,159,160,161, 161,162,163,164, 165,166,166,167, 168,169,170,170, //032-047
  171,172,173,173, 174,175,176,176, 177,178,179,179, 180,181,182,182, //048-063
  183,184,184,185, 186,187,187,188, 189,189,190,191, 191,192,193,193, //064-079
  194,195,195,196, 197,197,198,199, 199,200,200,201, 202,202,203,204, //080-095
  204,205,205,206, 207,207,208,208, 209,210,210,211, 211,212,213,213, //096-111
  214,214,215,215, 216,216,217,218, 218,219,219,220, 220,221,221,222, //112-127
};

/*
 * Speaker volume translation table
 *   value=log2(index/steps + 1)*range+min
 *       where index:  array index
 *             steps:  total array entries
 *             range:  usable Cirrus Logic values
 *             min:    min Cirrus Logic setting
 */

#define SPEAKER_VOL_STEPS	(128)
#define SPEAKER_VOL_MIN		(203)
#define SPEAKER_VOL_MAX		(251)
#define SPEAKER_VOL_RANGE	( SPEAKER_VOL_MAX - SPEAKER_VOL_MIN + 1 )

u8 speakerVolume[128] = {
  203,203,204,204, 205,205,206,206, 207,207,208,208, 209,209,210,210, //000-015
  211,211,212,212, 213,213,214,214, 215,215,216,216, 216,217,217,218, //016-031
  218,219,219,220, 220,220,221,221, 222,222,223,223, 223,224,224,225, //032-047
  225,225,226,226, 227,227,227,228, 228,229,229,229, 230,230,230,231, //048-063
  231,232,232,232, 233,233,233,234, 234,234,235,235, 235,236,236,236, //064-079
  237,237,237,238, 238,239,239,239, 239,240,240,240, 241,241,241,242, //080-095
  242,242,243,243, 243,244,244,244, 245,245,245,245, 246,246,246,247, //096-111
  247,247,248,248, 248,248,249,249, 249,250,250,250, 250,251,251,251, //112-127
};
