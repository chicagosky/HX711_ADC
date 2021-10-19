#include "driver/gpio.h"

//number of samples in moving average dataset, value must be 1, 2, 4, 8, 16, 32, 64 or 128.
#define SAMPLES 					16		//default value: 16

#if (SAMPLES  != 1) & (SAMPLES  != 2) & (SAMPLES  != 4) & (SAMPLES  != 8) & (SAMPLES  != 16) & (SAMPLES  != 32) & (SAMPLES  != 64) & (SAMPLES  != 128)
	#error "number of SAMPLES not valid!"
#endif

//adds extra sample(s) to the dataset and ignore peak high/low sample, value must be 0 or 1.
#define IGN_HIGH_SAMPLE 			1		//default value: 1
#define IGN_LOW_SAMPLE 				1		//default value: 1

/** add comment explaining below **/
#if (SAMPLES  == 1) & ((IGN_HIGH_SAMPLE  != 0) | (IGN_LOW_SAMPLE  != 0))
	#error "number of SAMPLES not valid!"
#endif

//microsecond delay after writing sck pin high or low. This delay could be required for faster mcu's.
//So far the only mcu reported to need this delay is the ESP32 (issue #35), both the Arduino Due and ESP8266 seems to run fine without it.
//Change the value to '1' to enable the delay.
#define SCK_DELAY					1		//default value: 0

//if you have some other time consuming (>60μs) interrupt routines that trigger while the sck pin is high, this could unintentionally set the HX711 
//into "power down" mode 
//if required you can change the value to '1' to disable interrupts when writing to the sck pin.
#define SCK_DISABLE_INTERRUPTS		0		//default value: 0

#define DATA_SET 	SAMPLES + IGN_HIGH_SAMPLE + IGN_LOW_SAMPLE // total samples in memory

#if 		(SAMPLES == 1)
#define 	DIVB 0
#elif 		(SAMPLES == 2)
#define 	DIVB 1
#elif 		(SAMPLES == 4)
#define 	DIVB 2
#elif  		(SAMPLES == 8)
#define 	DIVB 3
#elif  		(SAMPLES == 16)
#define 	DIVB 4
#elif  		(SAMPLES == 32)
#define 	DIVB 5
#elif  		(SAMPLES == 64)
#define 	DIVB 6
#elif  		(SAMPLES == 128)
#define 	DIVB 7
#endif

#define SIGNAL_TIMEOUT	100

// SCK PIN
#define PD_SCK_GPIO GPIO_NUM_18
// DOUT PIN
#define DOUT_GPIO   GPIO_NUM_19
// GAIN
#define GAIN 1