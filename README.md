# BST900-ESP32
ESP32 firmware to control BST900 Boost Converters remotely

[BST900](aliexpress.com/item/32838432319.html) boost converters are widely and cheaply available on AliExpress. They are sold under the brand name Ming-He among others, and there are also no-name clones available even cheaper.
They are claimed to operate up to 15A and 900W, but personally I would not operate them at that power without improvements to the cooling which I have detailed in my BST900 repository.

##Function
This firmware is offered as an evolution from [BST900](https://github.com/delboy711/BST900) which replaced the stock STM8 firmware and allowed control of a BST900 boost converter over a serial interface.

This new version replaces the BST900 daughter board with an ESP32 with or without an integrated TFT display and allows a BST900 
to be controlled remotely over a WiFi interface using MQTT, and for statistics to be gathered.
On power up the controller will revert to its saved state.

Local control is not currently supported. 

It has been designed to be used with a [TTGO T-Display board](aliexpress.com/item/33050667207.html), but is easily adapted for other display options (or none). An [ADS1115 ADC converter](aliexpress.com/item/32648046830.html) may optionally be used in preference to the onboard ESP32 ADC. The ADS1115 performs considerably better than the on board ADC in ESP32 and is highly recommended.
(All links to stores are examples only.)

In its initial version local control is not possible. If you are looking for a wider set of features, or for management of other types of converters check out 
[Drok-Juntek-on-steroids](https://github.com/rin67630/Drok-Juntek-on-steroids)

##Features

* Control over Voltage and Current over a wireless interface using MQTT.  Many variables may be controlled over MQTT including calibration.
* Statistics and alarms over MQTT
* Temperature measurement and alarms with an NTC thermistor attached to converter heatsink.
* Automatic fan control. Fan speed is proportional to either temperature or load current.
* Closed loop feedback to ensure output Voltage is accurate in Constant Voltage (CV) mode, and Output Current is accurate in Constant Current (CC) mode.
* Choice of display modes showing combinations of Input Voltage, Output Voltage, Output Current, Constant Current (CC) indication, and Temperature.
* Automatic shutdown on over temperature alarm, or on loss of communication with remote controller.
* Saving of all parameters in Flash memory. On power on, the controller will revert to this state.
* Over the Air software updates.


##Application
My home battery system is charged from AC mains. A BST900 Boost Converter is used to boost an AC to DC power supply output up to the battery voltage, and software
 running on NodeRed dynamically controls the constant current limit of the BST900 to match solar panel production to minimise export to the grid and save power for use later in the day.

##Compiling
This code compiles with PlatformIO IDE using the Arduino framework. Refer to the platformio.ini file for required libraries if using Arduino IDE.
Put your WIFI and MQTT credentials in the file inc/credentials.h, and your MQTT server address and topic details in inc/defines.h


##Usage
The BST900 will listen for commands on one MQTT topic, and send statistics every 30 seconds on another. By default it will time out and restart if no commands are 
received for two minutes. On power on it will send two messages giving its configuration and calibration settings.
Commands and statistics are JSON formatted like for example :-
<code>{"volts":45000,"amps":8000,"enable":true,"fan":0}</code>
Keywords may appear in any order. A list of keywords and their data types and ranges is in the file Keywords.md
If the keyword `"save":true` is given, all parameters are saved to flash memory and will be restored on power up.

##Hardware Connections
Connect the TTGO T-Display board to the ESP32 in the following way. BST900 pins are numbered from the top down.

|BST900 Left Hand Socket | TTGO-T-Display Pin | ADS1115 Pin | Function
| ------- | -------------|-----------------|------------------------------------|
| Pin 1 | Iout Sense | GPIO 38 | Chan 1 | Current sense. Use either ADS115 or ESP32 pins | 
| Pin 2 | Vout Sense | GPIO 39 | Chan 2 | Voltage out sense. |
| Pin 3 | Vin Sense  | GPIO 37 | Chan 0 | Voltage in sense. |
| Pin 4 | Iout PWM   | GPIO 13 |        | Constant Current threshold control |
| Pin 5 | Vout PWM   | GPIO 15 |        | Voltage out control |
| Pin 6 | Enable     | GPIO 13 |        | Converter Enable |
| Pin 7 | CV/CC Indicator | GPIO 27 |   | Constant Voltage/Constant Current Indicator |
| Pin 8 | Fan PWM    | GPIO 2  |        | Fan speed control |
|       |   SCL      | GPIO 22 | SCL    | I2C Clock |
|       |   SCD      | GPIO 21 | SCD    | I2C Data |
|       | ADC Ref    | GPIO 33 | +3.3V  | Connect to 3.3V power rail. Used to compare with NTC temp reading |
|       | Temp       | GPIO 36 |        | Connect up as +3.3V--10k resistor--GPIO 36--NTC Thermistor--GND |

Thermistor is [10K nominal at 25deg C Beta Coefficient=3950](https://lcsc.com/product-detail/NTC-Thermistors_Nanjing-Shiheng-Elec-MF52A103J3950-A1_C123378.html) or similar.

If using ESP32 ADC connect 100nF capacitors from GPIO Pins 37, 38, and 39 to GND.
 

BST900 Right Hand Socket | TTGO-T-Display Pin | ADS1115 Pin | Function
| ------- | -------------|-----------------|------------------------------------|
| Pin 1 | GND (Input) | GND | GND | Ground | 
| Pin 2 | GND (Input) | GND | GND | Ground |
| Pin 3 | GND (Output) |    |     | Do NOT CONNECT Connecting GNDs together disrupts Iout sense |
| Pin 4 | GND (Output) |    |     | Do NOT CONNECT Connecting GNDs together disrupts Iout sense |
| Pin 5 | +5V          | +5V  |    | 5V Power rail  |
| Pin 6 | +5V          | +5V  |    | 5V Power rail  |
| Pin 7 |              |      |    | Tx pin (not used) |
| Pin 8 |              |      |    | Rx pin (not used) |
|       |              | +3.3V | Vcc | ADS1115 power |

A 470 uF capacitor should be connected between +5V and GND as well as a 100nF capacitor.


