# HX711_ADC

This is a port of the HX711 ADC adapter created by Olav Kallhovd (https://github.com/olkal/HX711_ADC). The original library was written in Arduino but I wanted a CPP version written specifically for the ESP32 chip.

Compared to the original adapter, I have removed various optional elements and configurable items and made them static via a configuration header file. This simplifies the code and serves a minimal purpose. Use this if you need to interface an ESP32 chip with HX711 Load cell amplifier and prefer not to use the Arduino overheads.

Installation
The current files are in a self enclosed application. Download the entire repository into a folder and open using VSCode, compile and run. It presumes that you have ESP IDF already configured to work with VSCode. Change SCK and DOUT pin configurations in the Configurations header file.
