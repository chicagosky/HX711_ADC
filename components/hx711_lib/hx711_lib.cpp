#include "hx711_lib.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include <stdio.h>

// Error check the return of a function
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)

// Error check the validity of parameters
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#define SHIFTIN_WITH_SPEED_SUPPORT(data,clock) shiftInSlow(data,clock)

// Define thread safe resource indicator
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;


/*
 *  Default constructor
 */
hx711_lib::hx711_lib() {
    gpio_set_direction(DOUT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(PD_SCK_GPIO, GPIO_MODE_OUTPUT);
    powerUp();
}

//returns the current calibration factor
float hx711_lib::getCalFactor() {
	return calFactor;
}

float hx711_lib::getData() { // return fresh data from the moving average dataset
	long data = 0;
	lastSmoothedData = smoothedData();
	data = lastSmoothedData - tareOffset ;
	float x = (float)data * calFactorRecip;
	return x;
}

//returns 'true' if tareNoDelay() operation is complete
bool hx711_lib::getTareStatus() {
	bool t = tareStatus;
	tareStatus = 0;
	return t;
}

//for testing and debugging:
//returns the tare timeout flag from the last tare operation. 
//0 = no timeout, 1 = timeout
bool hx711_lib::getTareTimeoutFlag() {
	return tareTimeoutFlag;
}

//power down the HX711
void hx711_lib::powerDown() {
    gpio_set_level(PD_SCK_GPIO, 0);
    gpio_set_level(PD_SCK_GPIO, 1);
}

//power up the HX711
void hx711_lib::powerUp() {
	gpio_set_level(PD_SCK_GPIO, 0);
}

//set new calibration factor, raw data is divided by this value to convert to readable data
void hx711_lib::setCalFactor(float cal) {
	calFactor = cal;
	calFactorRecip = 1/calFactor;
}

uint8_t hx711_lib::shiftInSlow(uint8_t dataPin, uint8_t clockPin) {
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i) {
		gpio_set_level(PD_SCK_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(1)); 
        value |= gpio_get_level(DOUT_GPIO) << (7 - i);
        gpio_set_level(PD_SCK_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    return value;
}

/*  
 * start(t, dotare) with selectable tare:
 * will do conversions continuously for 't' +400 milliseconds (400ms is min. settling time at 10SPS). 
 * Running this for 1-5s in setup() - before tare() seems to improve the tare accuracy. 
 */
void hx711_lib::start(unsigned long t, bool dotare) {
	// adding min settle time to user specified delay
    t += 400;
    // getting the amount of time since boot
	lastDoutLowTime = esp_timer_get_time() / 1000;
    // if time since boot is less than delay, wait
	while((esp_timer_get_time() / 1000) < t) {
		update();
		taskYIELD();
	}

	if (dotare) {
		tare();
		tareStatus = 0;
	}
}

//zero the scale, wait for tare to finnish (blocking)
void hx711_lib::tare() {
	uint8_t rdy = 0;
	doTare = 1;
	tareTimes = 0;
	tareTimeoutFlag = 0;
	unsigned long timeout = (esp_timer_get_time() / 1000) + tareTimeOut;
	while(rdy != 2) 
	{
		rdy = update();
		if (!tareTimeoutDisable) {
			if ((esp_timer_get_time() / 1000) > timeout) { 
				tareTimeoutFlag = 1;
				break; // Prevent endless loop if no HX711 is connected
			}
		}
		taskYIELD();
	}
}

//zero the scale, initiate the tare operation to run in the background (non-blocking)
void hx711_lib::tareNoDelay() {
	doTare = 1;
	tareTimes = 0;
}


//call the function update() in loop or from ISR
//if conversion is ready; read out 24 bit data and add to dataset, returns 1
//if tare operation is complete, returns 2
//else returns 0
uint8_t hx711_lib::update() {
    uint8_t dout = gpio_get_level(DOUT_GPIO); 

	if (!dout) {
		conversion24bit();
		lastDoutLowTime = esp_timer_get_time() / 1000;
		signalTimeoutFlag = 0;
	} else {
		if ((esp_timer_get_time() / 1000) - lastDoutLowTime > SIGNAL_TIMEOUT) {
			signalTimeoutFlag = 1;
		}
		convRslt = 0;
	}
	return convRslt;
}

/*
 * 
 *  read 24 bit data, store in dataset and start the next conversion
 */
void hx711_lib::conversion24bit() {
	conversionTime = esp_timer_get_time() - conversionStartTime;
	conversionStartTime = esp_timer_get_time();
	convRslt = 0;
    unsigned long data = 0;
	uint8_t dout;
	
	if (SCK_DISABLE_INTERRUPTS) {
       portENTER_CRITICAL(&mux);
    }

	for (uint8_t i = 0; i < (24 + GAIN); i++) { //read 24 bit data + set gain and start next conversion
		if (SCK_DELAY) {
            vTaskDelay(pdMS_TO_TICKS(1)); // could be required for faster mcu's
        }
        
        gpio_set_level(PD_SCK_GPIO, 1); // Power up the SCK PIN
				
		if (SCK_DELAY) {
            vTaskDelay(pdMS_TO_TICKS(1)); // could be required for faster mcu's
        }
		
        gpio_set_level(PD_SCK_GPIO, 0); // Power down the SCK PIN

		if (i < (24)) {
			dout = gpio_get_level(DOUT_GPIO);  
			data = (data << 1) | dout;
		}
	}

	if (SCK_DISABLE_INTERRUPTS) {
        portEXIT_CRITICAL(&mux);
    }
	
	//The HX711 output range is min. 0x800000 and max. 0x7FFFFF (the value rolls over).
	//In order to convert the range to min. 0x000000 and max. 0xFFFFFF,
	//the 24th bit must be changed from 0 to 1 or from 1 to 0.
	data = data ^ 0x800000; // flip the 24th bit 
	
	if (data > 0xFFFFFF) {
		dataOutOfRange = 1;
	}

	if (readIndex == samplesInUse + IGN_HIGH_SAMPLE + IGN_LOW_SAMPLE - 1) {
		readIndex = 0;
    } else {
		readIndex++;
	}

	if(data > 0) {
		convRslt++;
		dataSampleSet[readIndex] = (long)data;
		if(doTare) {
			if (tareTimes < DATA_SET) {
				tareTimes++;
			} else {
				tareOffset = smoothedData();
				tareTimes = 0;
				doTare = 0;
				tareStatus = 1;
				convRslt++;
			}
		}
	}
}

long hx711_lib::smoothedData() {
	long data = 0;
	long L = 0xFFFFFF;
	long H = 0x00;
	for (uint8_t r = 0; r < (samplesInUse + IGN_HIGH_SAMPLE + IGN_LOW_SAMPLE); r++) {
		#if IGN_LOW_SAMPLE
		if (L > dataSampleSet[r]) {
            L = dataSampleSet[r]; // find lowest value
        }
		#endif
		#if IGN_HIGH_SAMPLE
		if (H < dataSampleSet[r]) {
            H = dataSampleSet[r]; // find highest value
        }
		#endif
		data += dataSampleSet[r];
	}
	#if IGN_LOW_SAMPLE 
	data -= L; //remove lowest value
	#endif
	#if IGN_HIGH_SAMPLE 
	data -= H; //remove highest value
	#endif

	return (data >> divBit);
}
