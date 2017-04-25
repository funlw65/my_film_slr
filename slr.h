/**  (c) Vasile Guta-Ciucur, funlw65@gmail.com, all rights reserved
 *   Licensed under the MIT license (see the LICENSE file).
 *
 *    D I Y   F I L M   S L R   C A M E R A
 *    -------------------------------------
 *    - using a Canon EOS EF Lens or a manual lens;
 *    - using Av, Tv, M modes;
 *    - using a TSL2591 I2C sensor - I have to port Adafruit lib to C language;
 *    - using an STM32L1 CortexM3 microcontroller.
 */ 

/** DISCLAIMER!!!
 *  =============
 * 
 *  USE YOUR CANON LENS AT YOUR OWN RISC! DON'T BLAME ME OR CANON IF YOU
 *  BRICK IT! IF YOU ARE NOT SURE, USE A MANUAL LENS!
 */

/** FOREWORDS
 *  ---------
 *  One of the two limitations I won't try to mendle is the 
 *  lightmeter sensor limitation. The range is from 0 EV to 15 EV. 
 *
 *  The second limitation is dictated by the shutter's hardware 
 *  capabilities. If is handmade (Kevin Kadooka's one leaf shutter), it 
 *  will have a maximum speed of 1/125 of a second. If is recovered from 
 *  a more recent SLR film camera, there will be no limitation up to 
 *  1/1000 or 1/2000 (unless is from a more capable fullframe Canon DSLR).
 *  And, anything bellow 1 sec. speed is considered bulb mode - here, the 
 *  firmware won't help you to calculate your long exposures.
 * 
 */

typedef enum { MANUAL = 0, EOS} lens_t;
typedef enum { EOS50MM12 = 0, EOS50MM14, EOS50MM18, EOS85MM12, EOS85MM18} eos_t;
typedef enum { IS = 0, MA, MT, AV, TV} cameramode_t;

/** film sensitivity in ISO values - 160 will be treated as 100 ISO */
const uint16_t ISO_values[8]={25,50,100,200,400,800,1600,3200};

/** Shutter speeds.
 *  the array index will be used to determine if microseconds!
 *  got it? if index is between 1 and 3 is about microseconds 
 *  (you will use a microsecond delay function).
 */
const uint16_t Tv_speed[]={
	777, /* speed error */
	125, /* 1/8000, value in microseconds,  */
	250, /* 1/4000,  -"- */
	500, /* 1/2000,  -"- */
	1,   /* 1/1000, value in milliseconds   */
	2,   /* 1/500,   -"- */
	4,   /* 1/250,   -"- */
	8,   /* 1/125,   -"- */
	17,  /* 1/60,    -"- */
	33,  /* 1/30,    -"- */
	67,  /* 1/15,    -"- */
	125, /* 1/8,     -"- */
	250, /* 1/4,     -"- */
	500, /* 1/2,     -"- */
	1000,/* 1,       -"- */
	0    /* Bulb mode    */
};

const uint16_t Tv_markings[]=
{
	777, /* speed error */ 
	8000,/* 1   */
	4000,/* 2   */  
	2000,/* 3   */
	1000,/* 4   */
	500, /* 5   */
	250, /* 6   */
	125, /* 7   */
	60,  /* 8   */
	30,  /* 9   */
	15,  /* 10  */
	8,   /* 11  */
	4,   /* 12  */
	2,   /* 13  */
	1,   /* 14  */
	0    /* 15  this one is for Bulb mode */
};


/** USER CONSTANTS - user settable.
 *  You must set the maximum speed of your shutter by specifying the 
 *  index inside the ST_speed[] array. Default is 4, 
 *  meaning ST_speed[4] = 1, meaning 1/1000 maximum shutter speed.
 *
 *  And you must set the minimum aperture value for the manual lens you will use
 *   it is 16 by default...
 *  I know, the shutter is a constant, a fixed mechanism, and a lens is a 
 *  variable, something you can change anytime, but let this be for a while.
 *  For the EOS lenses, this will be set automatically.
 */
const uint8_t Tv_max_speed = 4; 
const uint8_t Av_min_aperture = 9;

/** aperture values (exceptions 1.2=12 1.7=20 1.8=22)
 *                               8                                        88  96 104
 */                        /*0   1    2   3   4   5   6    7   8   9  10  11  12  13*/
const float  Av_values[]   ={0,  1,  1.4, 2, 2.8, 4, 5.6,  8, 11, 16, 22, 32, 45, 64};
/* Canon EF 50mm lenses */
const int8_t EOSEF50mm12[] ={9, 12,  16, 24, 32, 40, 48,  56, 64, 72, -1, -1, -1, -1};
const int8_t EOSEF50mm14[] ={10, 0,  16, 24, 32, 40, 48,  56, 64, 72, 80, -1, -1, -1};
const int8_t EOSEF50mm18[] ={10, 0,  22, 24, 32, 40, 48,  56, 64, 72, 80, -1, -1, -1};
/* Canon EF 85mm lenses */
const int8_t EOSEF85mm12[] ={9, 12,  16, 24, 32, 40, 48,  56, 64, 72, -1, -1, -1, -1};
const int8_t EOSEF85mm18[] ={10, 0,  22, 24, 32, 40, 48,  56, 64, 72, 80, -1, -1, -1};

/* ========================================================================== */
/* ======================== 25 ISO TABLES =================================== */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_25[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV14_25[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2}; 
const uint8_t AV02_25[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV28_25[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV04_25[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV56_25[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 
const uint8_t AV08_25[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7}; 
const uint8_t AV11_25[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8}; 
const uint8_t AV16_25[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9}; 
const uint8_t AV22_25[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10}; 
const uint8_t AV32_25[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11}; 
const uint8_t AV45_25[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12}; 
const uint8_t AV64_25[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_25[] = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0}; 
const uint8_t TV00012_25[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13}; 
const uint8_t TV00014_25[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9, 10, 11, 12}; 
const uint8_t TV00018_25[] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11}; 
const uint8_t TV00115_25[] = {0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10}; 
const uint8_t TV00130_25[] = {0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4,  5,  6,  7,  8,  9}; 
const uint8_t TV00160_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3,  4,  5,  6,  7,  8}; 
const uint8_t TV01125_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2,  3,  4,  5,  6,  7}; 
const uint8_t TV01250_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  2,  3,  4,  5,  6}; 
const uint8_t TV01500_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1,  2,  3,  4,  5}; 
const uint8_t TV11000_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  1,  2,  3,  4}; 
const uint8_t TV12000_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  1,  2,  3}; 
const uint8_t TV14000_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  2}; 
const uint8_t TV18000_25[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1}; 
/* ==================== END 25 ISO TABLES =================================== */


/* ========================================================================== */
/* ========================= 50 ISO TABLES ================================== */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_50[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0}; 
const uint8_t AV14_50[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV02_50[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2}; 
const uint8_t AV28_50[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV04_50[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV56_50[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV08_50[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 
const uint8_t AV11_50[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7}; 
const uint8_t AV16_50[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8}; 
const uint8_t AV22_50[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9}; 
const uint8_t AV32_50[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10}; 
const uint8_t AV45_50[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11}; 
const uint8_t AV64_50[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_50[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV00012_50[] = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13,  0}; 
const uint8_t TV00014_50[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9, 10, 11, 12, 13}; 
const uint8_t TV00018_50[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11, 12}; 
const uint8_t TV00115_50[] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10, 11}; 
const uint8_t TV00130_50[] = {0, 0, 0, 0, 0, 0, 1, 2, 3, 4,  5,  6,  7,  8,  9, 10}; 
const uint8_t TV00160_50[] = {0, 0, 0, 0, 0, 0, 0, 1, 2, 3,  4,  5,  6,  7,  8,  9}; 
const uint8_t TV01125_50[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2,  3,  4,  5,  6,  7,  8}; 
const uint8_t TV01250_50[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  2,  3,  4,  5,  6,  7}; 
const uint8_t TV01500_50[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1,  2,  3,  4,  5,  6}; 
const uint8_t TV11000_50[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  1,  2,  3,  4,  5}; 
const uint8_t TV12000_50[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  1,  2,  3,  4}; 
const uint8_t TV14000_50[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  2,  3}; 
const uint8_t TV18000_50[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1,  2}; 
/* ===================== END 50 ISO TABLES ================================== */



/* ========================================================================== */
/* ======================== 100 ISO TABLES ================================== */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_100[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0}; 
const uint8_t AV14_100[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0}; 
const uint8_t AV02_100[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV28_100[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2}; 
const uint8_t AV04_100[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV56_100[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV08_100[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV11_100[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 
const uint8_t AV16_100[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7}; 
const uint8_t AV22_100[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8}; 
const uint8_t AV32_100[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9}; 
const uint8_t AV45_100[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10}; 
const uint8_t AV64_100[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_100[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0}; 
const uint8_t TV00012_100[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV00014_100[] = {0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9, 10, 11, 12, 13,  0}; 
const uint8_t TV00018_100[] = {0, 0, 0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11, 12, 13}; 
const uint8_t TV00115_100[] = {0, 0, 0, 0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10, 11, 12}; 
const uint8_t TV00130_100[] = {0, 0, 0, 0, 0, 1, 2, 3, 4,  5,  6,  7,  8,  9, 10, 11}; 
const uint8_t TV00160_100[] = {0, 0, 0, 0, 0, 0, 1, 2, 3,  4,  5,  6,  7,  8,  9, 10}; 
const uint8_t TV01125_100[] = {0, 0, 0, 0, 0, 0, 0, 1, 2,  3,  4,  5,  6,  7,  8,  9}; 
const uint8_t TV01250_100[] = {0, 0, 0, 0, 0, 0, 0, 0, 1,  2,  3,  4,  5,  6,  7,  8}; 
const uint8_t TV01500_100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  1,  2,  3,  4,  5,  6,  7}; 
const uint8_t TV11000_100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  1,  2,  3,  4,  5,  6}; 
const uint8_t TV12000_100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  1,  2,  3,  4,  5}; 
const uint8_t TV14000_100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  2,  3,  4}; 
const uint8_t TV18000_100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1,  2,  3}; 
/* ==================== END 100 ISO TABLES ================================== */

/* ========================================================================== */
/* ======================== 200 ISO TABLES ================================== */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_200[] = {13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0}; 
const uint8_t AV14_200[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0}; 
const uint8_t AV02_200[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0}; 
const uint8_t AV28_200[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV04_200[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2}; 
const uint8_t AV56_200[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV08_200[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV11_200[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV16_200[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 
const uint8_t AV22_200[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7}; 
const uint8_t AV32_200[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8}; 
const uint8_t AV45_200[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9}; 
const uint8_t AV64_200[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_200[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0}; 
const uint8_t TV00012_200[] = {1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13,  0,  0,  0}; 
const uint8_t TV00014_200[] = {0, 1, 2, 3, 4, 5, 6, 7,  8,  9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV00018_200[] = {0, 0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11, 12, 13,  0}; 
const uint8_t TV00115_200[] = {0, 0, 0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10, 11, 12, 13}; 
const uint8_t TV00130_200[] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,  7,  8,  9, 10, 11, 12}; 
const uint8_t TV00160_200[] = {0, 0, 0, 0, 0, 1, 2, 3,  4,  5,  6,  7,  8,  9, 10, 11}; 
const uint8_t TV01125_200[] = {0, 0, 0, 0, 0, 0, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10}; 
const uint8_t TV01250_200[] = {0, 0, 0, 0, 0, 0, 0, 1,  2,  3,  4,  5,  6,  7,  8,  9}; 
const uint8_t TV01500_200[] = {0, 0, 0, 0, 0, 0, 0, 0,  1,  2,  3,  4,  5,  6,  7,  8}; 
const uint8_t TV11000_200[] = {0, 0, 0, 0, 0, 0, 0, 0,  0,  1,  2,  3,  4,  5,  6,  7}; 
const uint8_t TV12000_200[] = {0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  1,  2,  3,  4,  5,  6}; 
const uint8_t TV14000_200[] = {0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  2,  3,  4,  5}; 
const uint8_t TV18000_200[] = {0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1,  2,  3,  4}; 
/* ==================== END 200 ISO TABLES ================================== */

/* ========================================================================== */
/* ======================== 400 ISO TABLES ================================== */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_400[] = {12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0}; 
const uint8_t AV14_400[] = {13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0}; 
const uint8_t AV02_400[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0}; 
const uint8_t AV28_400[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0}; 
const uint8_t AV04_400[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV56_400[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3   2}; 
const uint8_t AV08_400[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV11_400[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV16_400[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV22_400[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 
const uint8_t AV32_400[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7}; 
const uint8_t AV45_400[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8}; 
const uint8_t AV64_400[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_400[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0}; 
const uint8_t TV00012_400[] = {2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13,  0,  0,  0,  0}; 
const uint8_t TV00014_400[] = {1, 2, 3, 4, 5, 6, 7,  8,  9, 10, 11, 12, 13,  0,  0,  0}; 
const uint8_t TV00018_400[] = {0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV00115_400[] = {0, 0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10, 11, 12, 13,  0}; 
const uint8_t TV00130_400[] = {0, 0, 0, 1, 2, 3, 4,  5,  6,  7,  8,  9, 10, 11, 12, 13}; 
const uint8_t TV00160_400[] = {0, 0, 0, 0, 1, 2, 3,  4,  5,  6,  7,  8,  9, 10, 11, 12}; 
const uint8_t TV01125_400[] = {0, 0, 0, 0, 0, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11}; 
const uint8_t TV01250_400[] = {0, 0, 0, 0, 0, 0, 1,  2,  3,  4,  5,  6,  7,  8,  9, 10}; 
const uint8_t TV01500_400[] = {0, 0, 0, 0, 0, 0, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9}; 
const uint8_t TV11000_400[] = {0, 0, 0, 0, 0, 0, 0,  0,  1,  2,  3,  4,  5,  6,  7,  8}; 
const uint8_t TV12000_400[] = {0, 0, 0, 0, 0, 0, 0,  0,  0,  1,  2,  3,  4,  5,  6,  7}; 
const uint8_t TV14000_400[] = {0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  2,  3,  4,  5,  6}; 
const uint8_t TV18000_400[] = {0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1,  2,  3,  4,  5}; 
/* ==================== END 400 ISO TABLES ================================== */

/* ========================================================================== */
/* ======================== 800 ISO TABLES ================================== */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_800[] = {11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0, 0, 0}; 
const uint8_t AV14_800[] = {12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0, 0, 0}; 
const uint8_t AV02_800[] = {13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0, 0, 0}; 
const uint8_t AV28_800[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1, 0, 0}; 
const uint8_t AV04_800[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2, 1, 0}; 
const uint8_t AV56_800[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3, 2, 1}; 
const uint8_t AV08_800[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4, 3, 2}; 
const uint8_t AV11_800[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5, 4, 3}; 
const uint8_t AV16_800[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6, 5, 4}; 
const uint8_t AV22_800[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7, 6, 5}; 
const uint8_t AV32_800[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8, 7, 6}; 
const uint8_t AV45_800[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9, 8, 7}; 
const uint8_t AV64_800[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10, 9, 8}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_800[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0,  0}; 
const uint8_t TV00012_800[] = {3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13,  0,  0,  0,  0,  0}; 
const uint8_t TV00014_800[] = {2, 3, 4, 5, 6, 7,  8,  9, 10, 11, 12, 13,  0,  0,  0,  0}; 
const uint8_t TV00018_800[] = {1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11, 12, 13,  0,  0,  0}; 
const uint8_t TV00115_800[] = {0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV00130_800[] = {0, 0, 1, 2, 3, 4,  5,  6,  7,  8,  9, 10, 11, 12, 13,  0}; 
const uint8_t TV00160_800[] = {0, 0, 0, 1, 2, 3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13}; 
const uint8_t TV01125_800[] = {0, 0, 0, 0, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12}; 
const uint8_t TV01250_800[] = {0, 0, 0, 0, 0, 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11}; 
const uint8_t TV01500_800[] = {0, 0, 0, 0, 0, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10}; 
const uint8_t TV11000_800[] = {0, 0, 0, 0, 0, 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9}; 
const uint8_t TV12000_800[] = {0, 0, 0, 0, 0, 0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8}; 
const uint8_t TV14000_800[] = {0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7}; 
const uint8_t TV18000_800[] = {0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6}; 
/* ==================== END 800 ISO TABLES ================================== */

/* ========================================================================== */
/* ======================== 1600 ISO TABLES ================================= */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_1600[] = {10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0}; 
const uint8_t AV14_1600[] = {11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0}; 
const uint8_t AV02_1600[] = {12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0}; 
const uint8_t AV28_1600[] = {13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0}; 
const uint8_t AV04_1600[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0}; 
const uint8_t AV56_1600[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0}; 
const uint8_t AV08_1600[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV11_1600[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2}; 
const uint8_t AV16_1600[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV22_1600[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV32_1600[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV45_1600[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 
const uint8_t AV64_1600[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_1600[] = {5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0,  0,  0}; 
const uint8_t TV00012_1600[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0,  0}; 
const uint8_t TV00014_1600[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0}; 
const uint8_t TV00018_1600[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0}; 
const uint8_t TV00115_1600[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0}; 
const uint8_t TV00130_1600[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV00160_1600[] = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0}; 
const uint8_t TV01125_1600[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}; 
const uint8_t TV01250_1600[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 
const uint8_t TV01500_1600[] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; 
const uint8_t TV11000_1600[] = {0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; 
const uint8_t TV12000_1600[] = {0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 
const uint8_t TV14000_1600[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8}; 
const uint8_t TV18000_1600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7}; 
/* ==================== END 1600 ISO TABLES ================================= */

/* ========================================================================== */
/* ======================== 3200 ISO TABLES ================================= */
/* ========================================================================== */

/** Aperture priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01_3200[] = { 9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0}; 
const uint8_t AV14_3200[] = {10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0}; 
const uint8_t AV02_3200[] = {11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0}; 
const uint8_t AV28_3200[] = {12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0}; 
const uint8_t AV04_3200[] = {13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0}; 
const uint8_t AV56_3200[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0}; 
const uint8_t AV08_3200[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0}; 
const uint8_t AV11_3200[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV16_3200[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2}; 
const uint8_t AV22_3200[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV32_3200[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV45_3200[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV64_3200[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 

/** Shutter priority tables with regard to EV value (Ev=[0..15]) 
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001_3200[] = {6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0,  0,  0,  0}; 
const uint8_t TV00012_3200[] = {5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0,  0,  0}; 
const uint8_t TV00014_3200[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0,  0}; 
const uint8_t TV00018_3200[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0,  0}; 
const uint8_t TV00115_3200[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0,  0}; 
const uint8_t TV00130_3200[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0}; 
const uint8_t TV00160_3200[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV01125_3200[] = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0}; 
const uint8_t TV01250_3200[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}; 
const uint8_t TV01500_3200[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 
const uint8_t TV11000_3200[] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; 
const uint8_t TV12000_3200[] = {0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; 
const uint8_t TV14000_3200[] = {0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 
const uint8_t TV18000_3200[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8}; 
/* ==================== END 3200 ISO TABLES ================================= */

/* -- Global variables -------------------------------------------------- */ 
uint8_t  SLR_EV; /* using only the integer values of it                   */
float    SLR_LUX;/* this comes form the TSL2591 library                   */
uint8_t  SLR_ISO;/* current ISO - index to the ISO_values[] array         */
uint8_t  SLR_Av; /* current aperture - index to the Av_values[] array     */
uint8_t  SLR_Tv; /* current shutter speed - index to the Tv_speed[] array */
lens_t   SLR_LensType; /* current lens type                               */
eos_t    SLR_EOSModel; /* current EOS lens model                          */
cameramode_t SLR_Mode; /* current camera mode set by user                 */
/* ---------------------------------------------------------------------- */

/** FUNCTIONS
 *  ---------
 */
 
/* Sets the initial values for the ISO, Aperture, Shutter speed, etc.
 *
 */ 
void slr_init(void){
	/* for start, lets set some safe values */
	SLR_ISO = 2; /* ISO 100 - a good start for ISO */
	SLR_Av  = 6; /* Aperture 5.6 - any lens have that */
	SLR_Tv  = 7; /* Shutter speed 1/125 */
	SLR_EV  = 13;/* Light is ok */
} 

/* It will read the sensor at a press of a button - returns a code for success
 *  and lights a green led when the exposure is read correctly.
 * Requires a functional light sensor library... 
 */
uint8_t read_exposure(void){
	/**/
	
}

/* sets the EV value from the LUX value returned by the light sensor */
void getEV(float LUXvalue){
	if(LUXvalue  < 3.5)                                    {SLR_EV = 0; return;} 
	if((LUXvalue > 3.4)     && (LUXvalue < 7))             {SLR_EV = 1; return;}
	if((LUXvalue > 6.9)     && (LUXvalue < 14))            {SLR_EV = 2; return;}
	if((LUXvalue > 13.9)    && (LUXvalue < 28))            {SLR_EV = 3; return;}
	if((LUXvalue > 27.9)    && (LUXvalue < 56))            {SLR_EV = 4; return;}
	if((LUXvalue > 55.9)    && (LUXvalue < 112))           {SLR_EV = 5; return;}
	if((LUXvalue > 111.9)   && (LUXvalue < 225))           {SLR_EV = 6; return;}
	if((LUXvalue > 224.9)   && (LUXvalue < 450))           {SLR_EV = 7; return;}
	if((LUXvalue > 449.9)   && (LUXvalue < 900))           {SLR_EV = 8; return;}
	if((LUXvalue > 899.9)   && (LUXvalue < 1800))          {SLR_EV = 9; return;}
	if((LUXvalue > 1799.9)  && (LUXvalue < 3600))          {SLR_EV =10; return;}
	if((LUXvalue > 3599.9)  && (LUXvalue < 7200))          {SLR_EV =11; return;}
	if((LUXvalue > 7199.9)  && (LUXvalue < 14400))         {SLR_EV =12; return;}
	if((LUXvalue > 14399.9) && (LUXvalue < 28900))         {SLR_EV =13; return;}
	if((LUXvalue > 28899.9) && (LUXvalue < 57800))         {SLR_EV =14; return;}
	if(LUXvalue  > 57799.9)                                {SLR_EV =15; return;}
}


void setEOSlens(){
	/**/
}

void setSLRmode(void){
	/**/
}

void setISOindex(uint8_t dir){
	/**/
}

/* sets the Av of the lens according to the selected lens */
void setAVindex(uint8_t dir){
	/**/
}

/* sets the Tv according to the hardware capabilities of the shutter. 
 * the maximum speed of the shutter must be declared by the user.
 */
void setTVindex(uint8_t dir){
	/**/
}


uint16_t getISOvalue(void){
	/**/
}

/* gets the index of aperture value if in TV mode */
uint8_t getAVindex(void){
	/**/
}

/* gets the index of shutter speed value if in AV mode */
uint8_t getTVindex(void){
	/**/
}




