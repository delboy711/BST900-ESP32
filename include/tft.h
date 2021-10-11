#ifndef tft_H_
#define tft_H_
#pragma once


TaskHandle_t Tft_Task = NULL;            //RTOS task handle for TFT display update


void Tft_Task_function( void * parameter);

#endif