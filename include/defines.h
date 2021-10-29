
#ifndef defines_H_
#define defines_H_

#pragma once

#include <Arduino.h>
#include <ADS1115_WE.h>

#define HOSTNAME "BST900"               //Set hostname for mDNS
#define MQTT_RX_TOPIC "bst900"          //Topic to listen on for commands
#define MQTT_TX_TOPIC "emon/bst900"     //Topic to send status messages to

#define VIN_MAX      60
#define VOUT_MAX     120
#define IOUT_MAX     15
#define FAN_MIN      90.0                 //Minimum PWM required to start Fan as a float
#define FAN_TEMP_MIN 28.0                 //Temp fan starts at
#define FAN_TEMP_MAX 50.0                 //Temp fan is at full speed


/**********************************************
 * ADC Setup
 * Either internal ESP32 or external ADS1115 ADC
 * May be used
 * 
 * ********************************************/

#define HAS_ADS1115                         //Uses ADS1115 ADC converter instead of ESP32 internal ADC

#ifdef HAS_ADS1115                            //Uses ADS1115 ADC converter instead of ESP32 internal ADC
// THESE PARAMETERS FOR ADS1115 ADC	
#define Vin_BIAS   	    0.0	    //TBD               ADS1115
#define Vin_RES   	    34.517   //Was 34.335	//Provisional        ADS1115
	
#define Vout_BIAS		0.0     //TBD               ADS1115
#define Vout_RES 		67.599  // was 67.894	// Provisional      ADS1115

#define Iout_BIAS		48      // was 41.0	// Provisional      ADS1115
#define Iout_RES 		5.250   // was 5.065	// TBD              ADS1115

#define ADS_Ch2_Range ADS1115_RANGE_2048
#define ADS_Ch1_Range ADS1115_RANGE_2048
#define ADS_Ch0_Range ADS1115_RANGE_2048

#define Vin_PIN   ADS1115_COMP_0_GND        //ADS1115 Channel0
#define Iout_PIN  ADS1115_COMP_1_GND        //ADS1115 channel1
#define Vout_PIN  ADS1115_COMP_2_GND        //ADS1115 Channel2

#else   //HAS_ADS1115                       //Uses ESP32 internal ADC
// THESE PARAMETERS FOR ESP32 ADC	
#define Vin_BIAS   	    0.0	     //TBD              ESP32
#define Vin_RES   	    34.335	 //TBD              ESP32
	
#define Vout_BIAS		0       //TBD               ESP32
#define Vout_RES 		67.894	// TBD              ESP32

#define Iout_BIAS		41.0	// Provisional      ESP32
#define Iout_RES 		5.065	// TBD              ESP32


#define Vin_PIN   37            //Vin               ESP32
#define Iout_PIN  38            //Iout              ESP32
#define Vout_PIN  39            //Vout              ESP32
#endif  //HAS_ADS1115

	
//#define PWM_Iout_BIAS	0.0		//
//#define PWM_Iout_STEP	0.0511	//TBD
#define PWM_Vout_BIAS	0.0		 //
#define PWM_Vout_STEP	0.0363	     //Provisional

#define I2C_SCL     22          // GPIO5 for I2C (Wire) System Clock
#define I2C_SDA     21          // GPIO4 for I2C (Wire) System Data
#define TFT_BL       4          // Display backlight control pin

#define BUTTON_UP    35         //not currently used
#define BUTTON_DOWN  0          //not currently used

/***********************************
 * NTC Thermistor variables
 * *********************************/
#define TEMP        36          //GPIO Pin to measure NTC temperature  Uncomment if not used
#define ADC_VREF    33          //ADC PIN to measure 3.3V rail to compare with NTC
#define SERIES      10000.0     //Series resistor in line with NTC  3.3V---SERIES---ADC---NTC----GND
#define NOMINAL     10000.0     //Nominal NTC resistance
#define BETA        3950.0      //Beta Coefficient of NTC from spec sheet
#define OVERTEMP    60          //Over temp threshold shutdown
#define OVERTEMP_TIMEOUT 180    //Number of secs after overtemp condition clears before re- enabling


/************************************
 * Hardware connections
 * **********************************/

#define PWM_V       15  // Pin GPIO15  (Vpwm)
#define PWM_I       13  // Pin GPIO13  (Ipwm)
#define ENA_PIN     12  // Enable pin
#define FAN_PIN     2
#define CC_CV_PIN   27

#define WIFI_TIMEOUT 30000   //Interval between checking for WiFi
#define MQTT_TIMEOUT 120   //Panic if no MQTT messages received in this time in secs Set to 0 to disable (minimum 20 secs)


/***************************************
 * Data Structures
 * *************************************/

struct Mysettings {            //Controller settings. Will be restored from saved on power on
    uint32_t Vout=0;                // Target Voltage Out in mV.  32 bits because it can exceed 65V
    uint16_t Iout=0;                // Target Current out in mA    
    bool enable=false;              //Controller enabled
    uint8_t display_mode=4;         //TFT display mode
    uint16_t bl_intensity=1024;     //TFT screen brightness
    uint16_t Vpwm;                  //PWM value for Voltage
    //uint16_t Ipwm;                  //PWM value for Current
    uint8_t fan=0;              //Fan speed override. Set to zero for auto fan
    uint8_t overtemp_thresh = OVERTEMP;
    uint32_t mqtt_timeout = MQTT_TIMEOUT * 1000;   //MQTT Timeout

    //Calibration Settings
    int16_t vin_bias = Vin_BIAS;
    float vin_res = Vin_RES;
    float vout_bias = Vout_BIAS;
    float vout_res = Vout_RES;
    float iout_bias = Iout_BIAS;
    float iout_res = Iout_RES;
    float pwm_vout_bias = PWM_Vout_BIAS;
    float pwm_vout_step = PWM_Vout_STEP;
    //float pwm_iout_bias = PWM_Iout_BIAS;
    //float pwm_iout_step = PWM_Iout_STEP;  
};

#ifdef HAS_ADS1115
struct Adcdata {
  uint16_t sample;  //ADC sample
  float smoothed; //Raw sample smoothed by exponential filter
  ADS1115_RANGE adcrange;	//Input ADC voltage range
  ADS1115_MUX adc_pin;		//ESP32 Pin for input
};
#else
struct Adcdata {
  uint16_t sample;  //ADC sample
  float smoothed; //Raw sample smoothed by exponential filter
  uint8_t adc_pin;		//ESP32 Pin for input
};
#endif

struct Mystate {                    //Current State of Controller
    uint16_t Vin=0;                  // Actual Voltage In in mV
    uint32_t Vout=0;                 // Actual Voltage Out in mV.  32 bits because it can exceed 65V
    int16_t Iout=0;                 // Actual Current out in mA
    uint16_t Vin_smoothed=0;         // Smoothed Voltage In in mV
    uint32_t Vout_smoothed=0;        // Smoothed Voltage Out in mV.  32 bits because it can exceed 65V
    int16_t Iout_smoothed=0;        // Smoothed Current out in mA
    int16_t constant_current=0;     // Constant current inout
    int16_t cc_smoothed =0;            //Smoothed constant current indicator
    uint8_t fan_pwm=0;              //Fan PWM state
    //uint16_t Vin=0;               // Target Voltage In in mV
    unsigned long mqtt_active;      //time last MQTT command received
    int8_t temperature;             // Temp in C of NTC thermistor
    bool overtemperature = false;   //Over temp alarm
    uint16_t adc_time;              //Time spent processing ADC. Used for debug
    uint8_t reset_reason;           //Code for why ESP32 was last restarted

#ifdef HAS_ADS1115
    struct Adcdata adc_chan[3] = {
        {0,0,ADS_Ch0_Range, Vin_PIN},
        {0,0,ADS_Ch1_Range, Iout_PIN},
        {0,0,ADS_Ch2_Range, Vout_PIN}
    };
#else
    struct Adcdata adc_chan[3] = {   //array of structures
        {0,0, Vin_PIN},
        {0,0, Iout_PIN},
        {0,0, Vout_PIN}
    };
#endif
};

#endif
