//Rewritten by Derek Jennings Dec '19 for ESP32 using 'Preferences' library.


#include "settings.h"
Preferences prefs;


void Settings::WriteConfigToPreferences(char* settings, int size) {

  prefs.begin("myprefs", false); // use "myprefs" namespace
  uint16_t checksum = CRC16::CalculateArray((uint8_t*)settings, size);
  prefs.putBytes("config", settings, size);
  prefs.putUInt("CRC", checksum);       //Save checksum
  prefs.end();
}


bool Settings::ReadConfigFromPreferences(char* settings, int size) {
  prefs.begin("myprefs");
  prefs.getBytes("config", settings, size);  //Read prefs
  uint16_t checksum = CRC16::CalculateArray((uint8_t*)settings, size);
  uint16_t existingChecksum = prefs.getUInt("CRC",0);
  prefs.end();
  //for(int i=0; i <16 ; i++) Serial.print(settings[i], HEX);
  if (checksum == existingChecksum) {
    return true;
  }
  //Original data is now corrupt so return FALSE
  return false;
}


void Settings::PreferencesFactoryDefault() {
  prefs.begin("myprefs", false);
  prefs.clear();
  prefs.end();
}
