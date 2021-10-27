//#include <Arduino.h>
#include "defines.h"
#include "credentials.h"
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <rom/rtc.h>
#include "settings.h"

AsyncMqttClient mqtt;


extern struct Mystate state;            //Global structure holding controller state
extern struct Mysettings settings;     //Global structure holding target voltage/current
extern TaskHandle_t Fan_Task;            //RTOS task handle for Fan management
extern TaskHandle_t Debug_Task;            //RTOS task handle for Debug mode

void Mqtt_Connect() {
  if (mqtt.connected() == false) {
    Serial.print("Connecting to MQTT server at ");
    Serial.println(MQTT_HOST);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCredentials(MQTT_USER, MQTT_PASSWORD);
    mqtt.connect();
  }

}


void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT");
  mqtt.subscribe(MQTT_RX_TOPIC, 0);
  Serial.println("Subscribing to bst900 ");

  /***********************************
   * Power on message
   * Send configuration
   * *********************************/
  char jsonbuffer[400];
  DynamicJsonDocument doc(400);
  JsonObject root = doc.to<JsonObject>();
  root["reset_reason"] = state.reset_reason;
  root["overtemp_thresh"] = settings.overtemp_thresh;
  root["mqtt_timeout"] = settings.mqtt_timeout/1000;
  root["bl_intensity"] = settings.bl_intensity;
  root["display_mode"] = settings.display_mode;
  root["enable"] = settings.enable;
  root["targetVout"] = settings.Vout;
  root["targetIout"] = settings.Iout;
  serializeJson(doc, jsonbuffer, sizeof(jsonbuffer));
  mqtt.publish(MQTT_TX_TOPIC, 0, false, jsonbuffer);

  char jsonbuf[400];
  DynamicJsonDocument docu(400);
  JsonObject calib = docu.to<JsonObject>();
  calib["vin_res"] = settings.vin_res;
  calib["vout_res"] = settings.vout_res;
  calib["iout_res"] = settings.iout_res;
  calib["iout_bias"] = settings.iout_bias;
  calib["pwm_vout_step"] = settings.pwm_vout_step;
  calib["pwm_vout_bias"] = settings.pwm_vout_bias;
  //calib["pwm_iout_step"] = settings.pwm_iout_step;
  //calib["pwm_iout_bias"] = settings.pwm_iout_bias;
  serializeJson(docu, jsonbuf, sizeof(jsonbuf));
  mqtt.publish(MQTT_TX_TOPIC, 0, false, jsonbuf);

} 

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT");
} 

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println();Serial.print("MQTT Command received of size: "); Serial.print(len); Serial.print("  ");
  for(uint16_t i=0; i< len; i++) Serial.print(payload[i]);     //necessary because payload has no null terminator
  Serial.println();
  StaticJsonDocument<512> mqtt_json;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(mqtt_json, payload);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    mqtt.publish(MQTT_TX_TOPIC, 0, true, "JSON Deserialise error");
    Serial.println(error.c_str());
    return;
  } else {
    state.mqtt_active = millis();              //Reset MQTT activity timer
  }

  if(mqtt_json.containsKey("volts")) {
    settings.Vout = (uint32_t)(constrain((mqtt_json["volts"].as<float>()), 0, VOUT_MAX)*1000.0);   //Convert from float volts to mV
    uint32_t Vpwm = uint32_t(settings.Vout * settings.pwm_vout_step + settings.pwm_vout_bias);
    //Vpwm = constrain( Vpwm, 0, 8192);     //Limit output as a precaution too high a value can Zap components!!
    if(Vpwm > 0x07ff) Vpwm = 0x7ff;     //Put upper bound for safety
    if(Vpwm != settings.Vpwm) ledcWrite(0, Vpwm);   // PWM value has changed so write to voltage control
    settings.Vpwm = Vpwm;       //Save latest PWM target
  } 

  if(mqtt_json.containsKey("amps")) {
    settings.Iout = (uint16_t)(constrain((mqtt_json["amps"].as<float>()), 0, IOUT_MAX)*1000.0);   //Convert from float amps to mA
    //uint16_t Ipwm = settings.Iout * settings.pwm_iout_step + settings.pwm_iout_bias;
    //if(Ipwm != settings.Ipwm) ledcWrite(3, Ipwm);   // PWM value has changed so write to current control
    //settings.Ipwm = Ipwm;       //Save latest PWM target 
    //xTaskNotify(Fan_Task, 0x00, eNotifyAction::eNoAction);    //Wake up Fan task   
  }

  if(mqtt_json.containsKey("fan")) {
    settings.fan = mqtt_json["fan"];                          //Set fan speed override
  }

  if(mqtt_json.containsKey("display")) {
    settings.display_mode = mqtt_json["display"];                                      //Set TFT display mode
  }

  if(mqtt_json.containsKey("enable")) {             //Enable/Disable Converter
    if(mqtt_json["enable"] == true) {
      settings.enable = true;
      if(state.overtemperature == false ) digitalWrite(ENA_PIN, 1);
    } else {
      settings.enable = false;
      digitalWrite(ENA_PIN, 0);
    }
  } 

  if(mqtt_json.containsKey("debug")) {
      xTaskNotify(Debug_Task, 0x00, eNotifyAction::eNoAction);    //Wake up Debug task
  } 

  //Calibration Setting
  if(mqtt_json.containsKey("vin_bias")) settings.vin_bias = mqtt_json["vin_bias"];
  if(mqtt_json.containsKey("vin_res")) settings.vin_res = mqtt_json["vin_res"];
  if(mqtt_json.containsKey("vout_bias")) settings.vout_bias = mqtt_json["vout_bias"];
  if(mqtt_json.containsKey("vout_res")) settings.vout_res = mqtt_json["vout_res"];
  if(mqtt_json.containsKey("iout_bias")) settings.iout_bias = mqtt_json["iout_bias"];
  if(mqtt_json.containsKey("iout_res")) settings.iout_res = mqtt_json["iout_res"];
  if(mqtt_json.containsKey("pwm_vout_bias")) settings.pwm_vout_bias = mqtt_json["pwm_vout_bias"];
  if(mqtt_json.containsKey("pwm_vout_step")) settings.pwm_vout_step = mqtt_json["pwm_vout_step"];
  //if(mqtt_json.containsKey("pwm_iout_bias")) settings.pwm_iout_bias = mqtt_json["pwm_iout_bias"];
  //if(mqtt_json.containsKey("pwm_iout_step")) settings.pwm_iout_step = mqtt_json["pwm_iout_step"];

  //Alarm thresholds
  if(mqtt_json.containsKey("mqtt_timeout")) {   //Minimum 20 secs timeout so we can fix it if bad value
   settings.mqtt_timeout = (mqtt_json["mqtt_timeout"].as<unsigned int>() > 20) ? mqtt_json["mqtt_timeout"].as<unsigned int>() * 1000 : 20000;     //PANIC if no mqtt in this many secs. Zero to disable
  if(mqtt_json.containsKey("overtemp_thresh")) settings.overtemp_thresh = mqtt_json["overtemp_thresh"]; //Alarm if > this temperature
  }

  //Restore factory defaults
  if(mqtt_json.containsKey("factory") && mqtt_json["factory"] == true) {
    //Save settings
    Settings::PreferencesFactoryDefault();
    delay(2000);
    ESP.restart();
  }

  //LAST BUT ONE ENTRY
  if(mqtt_json.containsKey("save") && mqtt_json["save"] == true) {
    //Save settings
    Settings::WriteConfigToPreferences((char *)&settings, sizeof(settings));
  }
  //LAST ENTRY
  if(mqtt_json.containsKey("restart") && mqtt_json["restart"] == true) {
    Serial.println("Restarting Controller");     //Restart ESP32
    delay(1000); 
    ESP.restart();
    }  
}



void Mqtt_Task_function( void * parameter) {

  mqtt.onConnect(onMqttConnect);            //Callback functions on MQTT Connection/Disconnection
  mqtt.onDisconnect(onMqttDisconnect);
  mqtt.onMessage(onMqttMessage);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    //Wait until ready to start (splash screen complete)

  for (;;) {     //Task loop

      vTaskDelay(pdMS_TO_TICKS(10000));   //10 sec delay

      char jsonbuffer[300];
      DynamicJsonDocument doc(300);
      JsonObject root = doc.to<JsonObject>();

      root["Vin"] = state.Vin;
      root["Vin_smoothed"] = state.Vin_smoothed;
      root["Vout"] = state.Vout;
      root["Vout_smoothed"] = state.Vout_smoothed;
      root["Iout"] = state.Iout;
      root["Iout_smoothed"] = state.Iout_smoothed;
      root["targetVout"] = settings.Vout;
      root["targetIout"] = settings.Iout;
      root["overtemp"] = state.overtemperature;
      root["fan"] = state.fan_pwm;  //Fan pwm 
#ifdef TEMP
      root["temperature"] = state.temperature;
#endif      
      root["enable"] = settings.enable;
      serializeJson(doc, jsonbuffer, sizeof(jsonbuffer));
      mqtt.publish(MQTT_TX_TOPIC, 0, false, jsonbuffer);

  }
    vTaskDelete( NULL );
}