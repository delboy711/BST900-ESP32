
/***********************************************
 * This task reads ADC inputs and
 * performs closed loop feedback to ensure output voltage
 * matches the target in CV mode, and output current matches the target in CC mode
 ***********************************************/

//#include <Arduino.h>
#include "defines.h"
#include <Ewma.h>
#ifdef HAS_ADS1115          //Use external ADC
#include <ADS1115_WE.h>
#endif



uint16_t Iout = 0;                  //Measured Iout in mA - local variable
uint32_t Vout = 0;                  //Measured Vout in mV - local variable
uint16_t Vin = 0;                   //Measured Vin in mV - local variable

float cc_smoothed;        //CV/CC indicator from BST900 smoothed

extern struct Mystate state;       //Global structure holding controller state
extern struct Mysettings settings;     //Global structure holding target voltage/current

Ewma IFilter(0.1);   // Exponential filter https://github.com/jonnieZG/EWMA weighting factor 0.1 for current
Ewma VinFilter(0.1);   // Filter  for voltage In
Ewma VoutFilter(0.1);   // Filter for voltage Out
Ewma ccFilter(0.025);  //Heavily smoothed filter of CV_CC indicator because it will toggle when in CV mode so we must find long term state

#ifdef HAS_ADS1115

ADS1115_WE adc(0x48);
/* functions to directly read/write to ADC registers
 *  
 */
uint8_t ADCwriteRegister(uint8_t reg, uint16_t val){
  Wire.beginTransmission(0x48);
  uint8_t lVal = val & 255;
  uint8_t hVal = val >> 8;
  Wire.write(reg);
  Wire.write(hVal);
  Wire.write(lVal);
  return Wire.endTransmission();
}
  
uint16_t ADCreadRegister(uint8_t reg){
  uint8_t MSByte = 0, LSByte = 0;
  uint16_t regValue = 0;
  Wire.beginTransmission(0x48);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(0x48,2);
  if(Wire.available()){
    MSByte = Wire.read();
    LSByte = Wire.read();
  }
  regValue = (MSByte<<8) + LSByte;
  return regValue;
}
#else

//Get ESP32 ADC sample
  float readADCmV(uint8_t adc_pin) {
	uint16_t sum = 0;
	for(int i=0; i< 9; i++) {
		sum += analogReadMilliVolts(adc_pin);
		//delay(1);
	}
	sum += analogRead(adc_pin);
	return ((1. * sum) / 10);
}
#endif  //HAS_ADS1115


// Sequence adc channels will be sampled in
// channels with closed loop feedback are weighted
const byte chan_seq[8] = {
                      1,      //chan 1 Iout
                      2,      //chan 2 Vout
                      1,      //chan 1 Iout
                      0,      //chan 0 Vin
                      1,      //chan 1 Iout
                      2,      //chan 2 Vout
                      1,      //chan 1 Iout
                      1       //chan 1 Iout
};
                  

void ADC_Task_function( void * parameter) {
  uint8_t chanindex=1;
  uint16_t Ipwm, Vpwm;
  uint16_t distance;
  unsigned long start_time;     //Time ADC processing starts

  pinMode(ENA_PIN, OUTPUT);
  pinMode(CC_CV_PIN, INPUT);
  ledcSetup(0, 2000, 13);         //Create 13 bit resolution@ 2Khz  PWM for Voltage
  ledcSetup(3, 2000, 13);         //Create 13 bit resolution@ 2Khz  PWM for Current
  ledcAttachPin(PWM_V, 0);        //Attach PWM channels to Pins
  ledcAttachPin(PWM_I, 3);
  ledcWrite(0, settings.Vpwm);              //Restore saved output voltage
  ledcWrite(3, 10);                         //Prime current limit
  digitalWrite(ENA_PIN, settings.enable);   //Restore saved enable state

#ifdef HAS_ADS1115
  Wire.begin();
  if(!adc.init()) {
    Serial.println("No ADS1115 present");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    //Idle task
  } else Serial.println("ADS1115 found");
#endif

  for (;;) {     //Task loop
  start_time = micros();    //Note time we start loop in microseconds
  uint8_t j = chan_seq[chanindex];

#ifdef HAS_ADS1115
  state.adc_chan[j].sample = adc.getResult_mV();   //Get latest sample from ADS1115
#else
  state.adc_chan[j].sample = (uint16_t)readADCmV(state.adc_chan[j].adc_pin);   //Get latest sample from ESP32 oversample 5 times
#endif  //HAS_ADS1115

  state.constant_current = !digitalRead(CC_CV_PIN);    //Read CC/CV Pin
  cc_smoothed = ccFilter.filter((state.constant_current));   //Find long term average of CC indicator
  state.cc_smoothed = round(cc_smoothed);
  
	switch (j) {
    case 0: //This sample is for Vin
      state.adc_chan[j].smoothed = VinFilter.filter(state.adc_chan[0].sample);
      Vin = (int16_t)(((float)(state.adc_chan[j].sample) - settings.vin_bias) * settings.vin_res);  //Get Vin in mV.
      state.Vin = Vin;      //Save state of Vin
      state.Vin_smoothed = round(((state.adc_chan[j].smoothed) - settings.vin_bias) * settings.vin_res);
      break;

		case 1:     // This sample is for Iout
     //   Closed loop feedback to adjust Current PWM based on results of last
     //   adc result. If in CC mode measured current is less than CC target then increment PWM pulse
     //   If in CV mode current drops, then decrement pulse. 
     //   Should cause constant current to converge onto its target without any PWM calibration.
     //   This fits closer to the observed behaviour of the stock firmware where the pulse
     //   width narrows in CV mode and expands in CC mode 
      state.adc_chan[j].smoothed = IFilter.filter(state.adc_chan[j].sample);
      Ipwm = ledcRead(3); //Get current duty cycle      
      Iout = (int16_t)(((float)(state.adc_chan[j].sample) - settings.iout_bias) * settings.iout_res);  //Get Iout in mAmps.
      if (Iout < 0.0 ) Iout=0.0;
      distance =  settings.Iout - Iout; //distance from target current in mA.
      distance = abs(distance);
        
      //If in Constant Current mode
      if ( state.constant_current ) {
			  //Increase PWM pulse proportionally if in CC and current below target.
			  if ( Iout < settings.Iout ) {  //Compare to target current if in CC node           
           Ipwm += (distance >> 5)+1;    //Increase current pulse proportional to distance between current and target
			  }
        
        // Reduce PWM pulse if in CC mode and current is above target
			  if ( Iout > settings.Iout + 10 && Iout > 0 ) {
				  if ((distance >> 5)+1 < Ipwm) {		//Avoid underflow
				  	Ipwm -= (distance >> 5)+1;  //Down speed proportional to distance between current and target
				  } else Ipwm -= 1;
			  }
		  }       
         
      //In constant Voltage mode
      if (!state.constant_current && Ipwm > 0) {
			
			  //Reduce PWM pulse if in CV mode current is above target
			  //if ( ( Iout + 256 < settings.Iout) || Iout > settings.Iout + 16 ) {
        if ( Iout > settings.Iout + 16 ) {  
				  Ipwm = ( Ipwm > ((distance >> 6) )) ? Ipwm - ((distance >> 6)+1) : Ipwm - 1;
				  //Ipwm -= 1;
			  }
		  }    
        
    if(Ipwm > 0xefff) Ipwm = 0;	//Must have underflowed
    if(Ipwm > 0x1fff) Ipwm = 0x1fff;	//Must have overflowed
    ledcWrite(3, Ipwm);   // Write PWM to current control
    state.Iout = Iout;    //Save state of Iout
    state.Iout_smoothed = round(((state.adc_chan[j].smoothed) - settings.iout_bias) * settings.iout_res);
		break;
		
	case 2: // This sample is for Vout
    state.adc_chan[j].smoothed = VoutFilter.filter(state.adc_chan[j].sample);
		Vpwm = ledcRead(0); //Get voltage PWM duty cycle
		Vout = (int32_t)(((float)(state.adc_chan[j].sample) - settings.vout_bias) * settings.vout_res);  //Get Vout in mV.

		if ( !state.constant_current ) {
      distance =  settings.Vout - Vout; //distance from target voltaget in mV.
      distance = abs(distance);
			if( Vout > ( settings.Vout + 3) && Vpwm > 0 ) {			//Vout too high, Avoid underflow
				Vpwm -= 1;									//Reduce PWM pulse
			} else {
				if( Vout < ( settings.Vout - 3) && Vpwm < 0x1fff ) {	//Vout too low, Avoid overflow
					Vpwm += 1;								//Increase PWM pulse
				} 
			}
      if (Vpwm < settings.Vpwm - 64) Vpwm = settings.Vpwm - 64;//Set limits for Vpwm auto adjustment
      if (Vpwm > settings.Vpwm + 64) Vpwm = settings.Vpwm + 64;
      if(Vpwm > 0x1fff) Vpwm = 0x1fff;     //Limit output as a precaution to avoid setting voltage too high
			ledcWrite(0, Vpwm);   // PWM to voltage control
		}
    state.Vout = Vout;      //Save state of Vout
    state.Vout_smoothed = round(((state.adc_chan[j].smoothed) - settings.vout_bias) * settings.vout_res);
		break;

	}

   // Start Next ADC measurement
   //*********************
   chanindex += 1;      //select next channel
   if (chanindex > 7) chanindex = 0;

   #ifdef HAS_ADS1115
   // 
   //   Write directly to registers to set up  a single conversion  because it is
   //   faster than the library
   // 

   uint16_t I2C_reg =  state.adc_chan[chan_seq[chanindex]].adcrange | state.adc_chan[chan_seq[chanindex]].adc_pin | ADS1115_SINGLE | ADS1115_128_SPS  | ADS1115_START_ISREADY ;
   ADCwriteRegister(1, I2C_reg); 
   #endif

    state.adc_time = micros() - start_time;  //Note how long we spent processing the loop for debug in microseconds

    vTaskDelay(10);    //wait for ADC to complete by pausing the task. RTOS will do something else during this time.
  }
  vTaskDelete( NULL );
}
