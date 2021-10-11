/****************************************
 * BST900-ESP32
 * ESP32 firmware for BST900 BoostConverters
 * 
 * Oct '21
 * Derek Jennings
 * 
 * ****************************************/


#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <rom/rtc.h>
#include "settings.h"


#include "defines.h"
#include "credentials.h"

#include "adc.h"
#include "mqtt.h"
#include "tft.h"

unsigned long wifi_check = 0;
IPAddress ip;


Mystate state;        //Global structure holding controller state
Mysettings settings;      //Global structure holding target voltage/current

TaskHandle_t Fan_Task = NULL;            //RTOS task handle for Fan management
TaskHandle_t Debug_Task = NULL;

void Debug_Task_function( void * parameter) {
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     //Wait until wake up

  for(;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));     //Wait 5 secs between reports

    Serial.print("Vin_sample= "); Serial.print(state.adc_chan[0].sample); Serial.print(" Vin= "); Serial.print(state.Vin);
    Serial.print(" Vout_sample= ");  Serial.print(state.adc_chan[2].sample); Serial.print(" Vout= "); Serial.print(state.Vout);
    Serial.print(" Iout_sample= ");  Serial.print(state.adc_chan[1].sample); Serial.print(" Iout= "); Serial.println(state.Iout);  
    Serial.print("Loop time = "); Serial.print((float)state.adc_time/1000.0); Serial.println(" mS");  
  }

}


//Get ESP32 ADC sample
  float readADC(uint8_t adc_pin) {
	uint16_t sum = 0;
	for(int i=0; i< 4; i++) {
		sum += analogReadMilliVolts(adc_pin);
		delay(20);
	}
	sum += analogRead(adc_pin);
	return ((1. * sum) / 5);
}

//Steinhart-Hart Equation to get Temperature from an NTC
// See https://arduinodiy.wordpress.com/2015/11/10/measuring-temperature-with-ntc-the-steinhart-hart-formula/
float ntc_temp(float vref  ) {    //vref= power rail sample, series = series resistor
  
  float adc_sample = readADC(TEMP);               //Get sample of V across NTC
  Serial.print("Sample = "); Serial.print(adc_sample); Serial.print("  Vref= "); Serial.println(vref);
  float Resistance = (vref / adc_sample) - 1.0;
  Resistance = SERIES / Resistance;
  Serial.print(Resistance); Serial.println(" Ohm");
  float steinhart;
  steinhart = Resistance / NOMINAL; // (R/Ro)
  steinhart = log(steinhart); // ln(R/Ro)
  steinhart /= BETA; // 1/B * ln(R/Ro)
  steinhart += 1.0 / (25 + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // Invert
  steinhart -= 273.15; // convert to C

  //Serial.print("Temperature ");
  //Serial.print(steinhart);
  //Serial.println(" oC");

  return steinhart;
}

void Fan_Task_function( void * parameter) {
  ledcSetup(14, 25, 8);             // Create 8 bit resolution@ 20Khz  PWM for Fan
  ledcAttachPin(FAN_PIN, 14);          //Attach PWM channel to fan pin
  float vref_sample = readADC(ADC_VREF);    //Measure 3.3V rail voltage as a reference
  unsigned long overtemp_timer=millis();

  for(;;) {

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(3000));     //Wait 3 secs for wake up and then proceed


#ifdef TEMP       //Temperature controlled fan
    state.temperature = round(ntc_temp(vref_sample));
    if (state.temperature > settings.overtemp_thresh) {    //Temperature too high!!!
      state.overtemperature = true;  //Raise alarm
      digitalWrite(ENA_PIN, 0);     //Disable Controller
      overtemp_timer = millis();    //Make note of time
    } else {
      if(state.overtemperature) {   //Still in alarm state
        unsigned long time_now = millis();
        if( time_now - overtemp_timer > OVERTEMP_TIMEOUT) {   //Timer has expired OK to reenable
          if(settings.enable) digitalWrite(ENA_PIN, 1);     //Enable Controller
          state.overtemperature = false;
        }
      }
    }
    //state.fan_pwm = constrain(map(state.temperature, 28, 50, FAN_MIN, 255), 0, 255);
    float temp_fan = ((float)state.temperature - FAN_TEMP_MIN) * (255.0 - FAN_MIN) / (FAN_TEMP_MAX - FAN_TEMP_MIN) + FAN_MIN;   //map temp range to fan range
    if ( temp_fan < FAN_MIN ) state.fan_pwm =0;
    else if ( temp_fan > 255.0 ) state.fan_pwm =255;
    else state.fan_pwm = (uint8_t)temp_fan;
#else                   //Fan speed dependent on current
    state.fan_pwm = constrain(map(state.Iout, 2500, 9000, FAN_MIN, 255), 0, 255);
    if(state.Iout < 2000) state.fan_pwm = 0;
#endif
    if(state.fan_pwm < FAN_MIN) state.fan_pwm = 0;   //Too low to start fan
    if (settings.fan == 0) {
      ledcWrite(14, state.fan_pwm );
  } else ledcWrite(14, settings.fan);   //Set Fan override speed
  }
}

void SetupOTA() {
  ArduinoOTA.onStart([]() { String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
      else
      type = "filesystem";
      ESP_LOGI(TAG, "Start updating %s", type);
  });
  ArduinoOTA.onEnd([]() { ESP_LOGD(TAG, "\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { ESP_LOGD(TAG, "Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error) {
    ESP_LOGD(TAG, "Error [%u]: ", error);
    if (error == OTA_AUTH_ERROR) ESP_LOGE(TAG, "Auth Failed");
    else if (error == OTA_BEGIN_ERROR) ESP_LOGE(TAG, "Begin Failed");
    else if (error == OTA_CONNECT_ERROR) ESP_LOGE(TAG, "Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) ESP_LOGE(TAG, "Receive Failed");
    else if (error == OTA_END_ERROR) ESP_LOGE(TAG, "End Failed");
    });

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setMdnsEnabled(true);
  ArduinoOTA.begin();
}


void Wifi_Connect() {
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}


void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.print("Got IP address:  ");
  ip = WiFi.localIP();
  Serial.println(WiFi.localIP().toString());
  Mqtt_Connect();       //Connect Mqtt Client if not already working
  MDNS.begin(HOSTNAME);
  xTaskNotify(Tft_Task, 0x00, eNotifyAction::eNoAction);
  SetupOTA();           //Setup ESP32 OTA Updates
}

void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info)
{

  Wifi_Connect();   //Reconnect

}


void setup() {

  Serial.begin(115200);
  delay(1000);
  state.reset_reason = rtc_get_reset_reason(0);
  Serial.print("Reset Reason = "); Serial.println(state.reset_reason);

//Restore Configuration from Flash
  Mysettings testsettings;
  if (Settings::ReadConfigFromPreferences((char *)&testsettings, sizeof(testsettings))) {  //Test if config is good before loading into running config
    Settings::ReadConfigFromPreferences((char *)&settings, sizeof(settings));
    Serial.println("Configuration OK");
  } else {
    Settings::PreferencesFactoryDefault();
    Settings::WriteConfigToPreferences((char *)&settings, sizeof(settings));    //Write default config
  }
  
  xTaskCreatePinnedToCore( ADC_Task_function,  "ADC_Task",  2048,  NULL,  4,   &ADC_Task, 1);  //This is the task to read the ADC
  xTaskCreatePinnedToCore( Mqtt_Task_function,  "Mqtt_Task",  4096,  NULL,  3,   &Mqtt_Task, 1);  //This is the task to handle MQTT commands
  xTaskCreatePinnedToCore( Fan_Task_function,  "Fan_Task",  1024,  NULL,  5,   &Fan_Task, 1);  //This is the task to handle the Fan
  xTaskCreatePinnedToCore( Tft_Task_function,  "Tft_Task",  4096,  NULL,  2,   &Tft_Task, 1);  //This is the task to handle TFT display
  xTaskCreatePinnedToCore( Debug_Task_function,  "Debug_Task",  2048,  NULL,  4,   &Debug_Task, 1);  //This is the task to output Debug messages

  //LoadConfig();
  WiFi.onEvent(onWifiConnect, system_event_id_t::SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(onWifiDisconnect, system_event_id_t::SYSTEM_EVENT_STA_DISCONNECTED);

  Wifi_Connect();
  
  state.mqtt_active = millis();     //Prime MQTT activity timer
}

void loop() {
  ArduinoOTA.handle();
  
  unsigned long millis_now = millis();
  if( millis_now - wifi_check > WIFI_TIMEOUT ) {  //Every 30 secs ensure we are connected to Wifi
    Wifi_Connect();                               //Does nothing if already connected
    Serial.println("Still Connected");
    wifi_check = millis_now;
  }
  if(millis_now - state.mqtt_active > settings.mqtt_timeout && settings.mqtt_timeout > 0) {  //Check we still have contact with MQTT
    Serial.println("PANIC!! Lost contact with MQTT Control. Rebooting");
    delay(3000);
    ESP.restart();
  }

  delay(100);
  yield();
}