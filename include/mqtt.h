#ifndef mqtt_H_
#define mqtt_H_
#pragma once

//#include <Arduino.h>


//RTOS Task to handle mqtt
TaskHandle_t Mqtt_Task = NULL;      //This is the task handle for the ADC task


void Mqtt_Task_function( void * parameter);
void Mqtt_Connect();
//void onMqttConnect(AsyncMqttClientConnectReason reason);
//void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)



#endif