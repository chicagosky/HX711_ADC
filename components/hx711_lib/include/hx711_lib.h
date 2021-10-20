#include "cstdint"
#include "configuration.h"

class hx711_lib {
    public:
        hx711_lib();                                // constructor
        float getCalFactor(); 						//returns the current calibration factor
        float getData(); 							//returns data from the moving average dataset 
        bool getTareTimeoutFlag();					//for testing and debugging
        bool getTareStatus();						//returns 'true' if tareNoDelay() operation is complete
        void powerDown(); 							//power down the HX711
		void powerUp(); 							//power up the HX711
        void setCalFactor(float cal); 				//set new calibration factor, raw data is divided by this value to convert to readable data
		void start(unsigned long t, bool dotare);	//start HX711, do tare if selected
        void tare(); 								//zero the scale, wait for tare to finnish (blocking)
        void tareNoDelay(); 						//zero the scale, initiate the tare operation to run in the background (non-blocking)
        uint8_t update(); 							//if conversion is ready; read out 24 bit data and add to dataset
    
    protected:
        float calFactor = 1.0;						//calibration factor as given in function setCalFactor(float cal)
		float calFactorRecip = 1.0;					//reciprocal calibration factor (1/calFactor), the HX711 raw data is multiplied by this value
        unsigned long conversionStartTime;
		unsigned long conversionTime;
        uint8_t convRslt;
        bool dataOutOfRange = 0;                    // Indicates if read value from HX711 is within range
        volatile long dataSampleSet[DATA_SET + 1];	// dataset, make voltile if interrupt is used 
        uint8_t divBit = DIVB;
        bool doTare;
        unsigned long lastDoutLowTime = 0;
        long lastSmoothedData = 0;
        int readIndex = 0;
        int samplesInUse = SAMPLES;
        bool signalTimeoutFlag = 0;
        long tareOffset;	                        // Offset value when Tare is performed
        bool tareStatus;
        unsigned int tareTimeOut = (SAMPLES + IGN_HIGH_SAMPLE + IGN_HIGH_SAMPLE) * 150; // tare timeout time in ms, no of samples * 150ms (10SPS + 50% margin)
        bool tareTimeoutFlag;
		bool tareTimeoutDisable = 0;
        uint8_t tareTimes;
        void conversion24bit(); 					//if conversion is ready: returns 24 bit data and starts the next conversion
        uint8_t shiftInSlow(uint8_t dataPin, uint8_t clockPin);
        long smoothedData();						//returns the smoothed data value calculated from the dataset
};