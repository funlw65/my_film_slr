/**  (c) Vasile Guta-Ciucur, funlw65@gmail.com
 *   Under the MIT license (see the LICENSE file).
 *
 *    D I Y   F I L M   S L R   C A M E R A
 *    -------------------------------------
 *    - using a Canon EOS EF Lens or a manual lens;
 *    - using Av, Tv, M modes;
 *    - using a TSL2591 I2C sensor - I have to port Ladyada lib to C language;
 *    - using an STM32L1 CortexM3 microcontroller.
 */ 

/** FOREWORDS
 *  ---------
 *  Ha! Let's start with a lame excuse: I have no idea what to do yet!
 *  But, considering that my DIY SLR has to emulate a mechanical film
 *  camera, I have to go with full stops and extending this limitation
 *  (he he, can you really extend a limitation?) over the EOS lens 
 *  as well.
 * 
 *  Still, why full stops when I have already the 1/3 values 
 *  defined in the Canon DSLR camera for aperture and shutter speed? 
 *  For film, the "Sunny 16" rule works with full stops and I have 
 *  already the table reference so, it is simple, will do for now. 
 * 
 *  Excuses, excuses... For 1/3 stops I should invent new EV values...
 *  Tempting! But lets stay faithful to our objectives.
 * 
 *  BTW! One of the two limitations I won't try to mendle is the 
 *  lightmeter sensor limitation. The range is from 0 EV to 15 EV. 
 *  The second limitation is dictated by the shutter's hardware 
 *  capabilities. If is handmade (Kevin Kadooka's one leaf shutter), it 
 *  will have a maximum speed of 1/125 of a second. If is recovered from 
 *  a more recent SLR film camera, there will be no limitation up to 
 *  1/1000 or 1/2000 (unless is from a more capable fullframe Canon DSLR).
 * 
 */
 
/** DOCUMENTATION 
 *  =============
 */
 
/* SHUTTER SPEEDS in micro and milliseconds
1/8000 = 125 microseconds
1/4000 = 250 -""-
1/2000 = 500 
1/1000 = 1   millisecond
1/500  = 2   -""-
1/250  = 4
1/125  = 8
1/60   = 17  
1/30   = 33
1/15   = 67
1/8    = 125
1/4    = 250
1/2    = 500
1      = 1000
*/

/* EV to LUX table at 100 ISO
1 	 5.00 	
1.5  7.00 	
2 	 10.00 	
2.5  14.00 	
3 	 20.00 	
3.5  28.00 	
4 	 40.00 	
4.5  56.00 	
5 	 80.00 	
5.5  112.00 	
6 	 160.00 	
6.5  225.00 	
7 	 320.00 	
7.5  450.00 	
8 	 640.00 	
8.5  900.00 	
9 	 1,280.00 	
9.5  1,800.00 	
10 	 2,600.00 	
10.5 3,600.00 	
11 	 5,120.00 	
11.5 7,200.00 	
12 	 10,240.00 	
12.5 14,400.00 	
13 	 20,480.00 	
13.5 28,900.00 	
14 	 40,960.00 	
14.5 57,800.00 	
15 	 81,900.00 	
15.5 116,000.00 	
16 	 164,000.00 	
16.5 232,000.00 	
17 	 328,000.00 	
17.5 464,000.00 	
18 	 656,000.00
*/

typedef enum { MANUAL = 0, EOS} lens_t;
typedef enum { EOS50MM12 = 0, EOS50MM14, EOS50MM18, EOS85MM12, EOS85MM18} eos_t;
typedef enum { M = 0, AV, TV} cameramode_t;

/* Global variables */ 
uint8_t  EV; /* using only the integer values of it */
float    LUX;/* this comes form the TSL2591 library */
uint16_t ISO;/* current ISO - index to the ISO_values[] array */
uint8_t  Av; /* current aperture - index to the Av_values[] array */
uint8_t  Tv; /* current shutter speed - index to the ST_speed[] array */


lens_t   LensType; /* current lens type */
eos_t    EOSModel; /* current EOS lens model */
cameramode_t Mode; /* current camera mode set by user */

/** film sensitivity in ISO values - 160 will be treated as 100 ISO */
const uint16_t ISO_values[]={ 0,25,50,100,200,400,800,1600,3200};
const int8_t   ISO_incr[] = {-3,-2,-1,  0,  1,  2,  3,   4,   5};

/** Shutter speeds.
 *  the array index will be used to determine if microseconds!
 *  you don't get it yet? if index is between 1 and 3 is about
 *  microseconds (you will use a microsecond delay function).
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
					0    /* Bulb mode*/};

const uint16_t Tv_markings[]={
					777, /* speed error */ 
					8000, 		/* 1   */
					4000,       /* 2   */  
					2000,		/* 3   */
					1000,		/* 4   */
					500,		/* 5   */
					250,		/* 6   */
					125,		/* 7   */
					60,			/* 8   */
					30,			/* 9   */
					15,			/* 10  */
					8,			/* 11  */
					4,			/* 12  */
					2,			/* 13  */
					1,			/* 14  */
					0  /* this one is for Bulb mode */};


/** USER CONSTANT - user settable.
 *  You must set the maximum speed of your shutter by specifying the 
 *  index inside the ST_speed[] array. Default is 4, 
 *  meaning ST_speed[4] = 1, meaning 1/1000 maximum shutter speed.
 */
const uint8_t Tv_max_speed = 4; 

/** aperture values (exceptions 1.2=12 1.7=20 1.8=22)
 *                               8                                        88  96 104
 */                        /*0   1    2   3   4   5   6    7   8   9  10  11  12  13*/
const float  Av_values[]   ={0,  1,  1.4, 2, 2.8, 4, 5.6,  8, 11, 16, 22, 32, 45, 64};
/* Canon EF 50mm lenses */
const int8_t EOSEF50mm12[] ={9, 12,  16, 24, 32, 40, 48,  56, 64, 72,  0,  0,  0,  0};
const int8_t EOSEF50mm14[] ={10, 0,  16, 24, 32, 40, 48,  56, 64, 72, 80,  0,  0,  0};
const int8_t EOSEF50mm18[] ={10, 0,  22, 24, 32, 40, 48,  56, 64, 72, 80,  0,  0,  0};
/* Canon EF 85mm lenses */
const int8_t EOSEF85mm12[] ={9, 12,  16, 24, 32, 40, 48,  56, 64, 72,  0,  0,  0,  0};
const int8_t EOSEF85mm18[] ={10, 0,  22, 24, 32, 40, 48,  56, 64, 72, 80,  0,  0,  0};

/** Aperture priority tables prior to EV value (Ev=[0..15]) at ISO 100
 *
 *  The array will contain the indices in Tv_speed[] array.
 */
const uint8_t AV01[] = {14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0}; 
const uint8_t AV14[] = {15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0}; 
const uint8_t AV02[] = {15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1}; 
const uint8_t AV28[] = {15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2}; 
const uint8_t AV04[] = {15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3}; 
const uint8_t AV56[] = {15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4}; 
const uint8_t AV08[] = {15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5}; 
const uint8_t AV11[] = {15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6}; 
const uint8_t AV16[] = {15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8,  7}; 
const uint8_t AV22[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9,  8}; 
const uint8_t AV32[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10,  9}; 
const uint8_t AV45[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10}; 
const uint8_t AV64[] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11}; 

/** Shutter priority tables prior to EV value (Ev=[0..15]) at ISO 100
 *
 *  The array will contain the indices in Av_values[] array.
 */
const uint8_t TV00001[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0,  0,  0}; 
const uint8_t TV00012[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13,  0,  0}; 
const uint8_t TV00014[] = {0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9, 10, 11, 12, 13,  0}; 
const uint8_t TV00018[] = {0, 0, 0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11, 12, 13}; 
const uint8_t TV00115[] = {0, 0, 0, 0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10, 11, 12}; 
const uint8_t TV00130[] = {0, 0, 0, 0, 0, 1, 2, 3, 4,  5,  6,  7,  8,  9, 10, 11}; 
const uint8_t TV00160[] = {0, 0, 0, 0, 0, 0, 1, 2, 3,  4,  5,  6,  7,  8,  9, 10}; 
const uint8_t TV01125[] = {0, 0, 0, 0, 0, 0, 0, 1, 2,  3,  4,  5,  6,  7,  8,  9}; 
const uint8_t TV01250[] = {0, 0, 0, 0, 0, 0, 0, 0, 1,  2,  3,  4,  5,  6,  7,  8}; 
const uint8_t TV01500[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  1,  2,  3,  4,  5,  6,  7}; 
const uint8_t TV11000[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  1,  2,  3,  4,  5,  6}; 
const uint8_t TV12000[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  1,  2,  3,  4,  5}; 
const uint8_t TV14000[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  2,  3,  4}; 
const uint8_t TV18000[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  1,  2,  3}; 



/** FUNCTIONS
 *  ---------
 */

/* sets the EV value from the LUX value returned by the light sensor */
uint8_t getEV(float LUXvalue){
	/**/
};

/* sets the index of aperture value if in TV mode */
uint8_t getAVindex(void){
	/**/
};

/* sets the index of shutter speed value if in AV mode */
uint8_t getTVindex(void){
	/**/
};



 
