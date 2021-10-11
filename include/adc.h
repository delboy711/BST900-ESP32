#ifndef adc_H_
#define adc_H_

#pragma once

//#include <Arduino.h>

//RTOS Task to read the ADC
TaskHandle_t ADC_Task = NULL;      //This is the task handle for the ADC task

uint8_t ADCwriteRegister(uint8_t reg, uint16_t val);
uint16_t ADCreadRegister(uint8_t reg);
void ADC_Task_function( void * parameter);



#endif