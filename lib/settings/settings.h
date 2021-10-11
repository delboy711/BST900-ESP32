#ifndef Settings_H // include guard
#define Settings_H

/*

EEPROM Settings storage with checksum (works with ESP8266 devices)
(c)2019 Stuart Pittaway

LICENSE
Attribution-NonCommercial-ShareAlike 2.0 UK: England & Wales (CC BY-NC-SA 2.0 UK)
https://creativecommons.org/licenses/by-nc-sa/2.0/uk/

* Non-Commercial — You may not use the material for commercial purposes.
* Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
  You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
* ShareAlike — If you remix, transform, or build upon the material, you must distribute your
  contributions under the same license as the original.
* No additional restrictions — You may not apply legal terms or technological measures
  that legally restrict others from doing anything the license permits.

Modified by Derek Jennings Dec '19 for ESP32 using 'Preferences' library.

*/

//#include <EEPROM.h>
#include <Preferences.h>

#include "crc16.h"

class Settings {
   public:
      static void WriteConfigToPreferences(char* settings, int size);
      static bool ReadConfigFromPreferences(char* settings, int size);
      static void PreferencesFactoryDefault();
};
#endif
