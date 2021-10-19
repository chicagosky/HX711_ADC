/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <hx711_lib.h>

extern "C" {
    void app_main();
}

const int calVal_calVal_eepromAdress = 0;
unsigned long t = 0;

hx711_lib loadCell;

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(10000)); // delay 10 milliseconds or 100000 microseconds
    printf("\n");
    printf("Starting...\n");

    float calibrationValue = 696.0; // uncomment this if you want to set this value in the sketch
    loadCell = hx711_lib();

    unsigned long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
    loadCell.start(stabilizingtime, true);
    if (loadCell.getTareTimeoutFlag()) {
        printf("Timeout, check MCU>HX711 wiring and pin designations\n");
    } else {
        loadCell.setCalFactor(calibrationValue); // set calibration factor (float)
        printf("Startup is complete\n");
    }
  
    while (!loadCell.update());
    printf("Calibration value: %f\n", loadCell.getCalFactor());

    static bool newDataReady = 0;
    const int serialPrintInterval = 500; //increase value to slow down serial print activity
    while (1) {
        // check for new data/start next conversion:
        if (loadCell.update()) {
            newDataReady = true;
        }

        // get smoothed value from the dataset:
        if (newDataReady) {
            if ((esp_timer_get_time() / 1000) > t + serialPrintInterval) {
                float i = loadCell.getData();
                printf("Load_cell output val: %f\n", i);
                newDataReady = 0;
                t = esp_timer_get_time() / 1000;
            }
        }
        
        // receive command from serial terminal, send 't' to initiate tare operation:
        /*if (Serial.available() > 0) {
            char inByte = Serial.read();
            if (inByte == 't') {
                loadCell.tareNoDelay();
            }
        }*/
        
        // check if last tare operation is complete:
        if (loadCell.getTareStatus() == true) {
            printf("Tare complete\n");
        }

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}