# MQTT Keywords
This is a list of the JSON keywords used in BST900-ESP32

## Command Keywords


| Keyword         | Data Type | Range      | Comment                                    |
| :-------------: | :-------: | :---------- | :---------------------------------------- |
| amps            | float      | 1-15.0     | Constant current threshold in A |
| debug           | boolean   | true/false  | Puts extra debug data on serial interface |
| display         | integer   | 0 to 4      | Contents of TFT display. 0=blank, 1=Voltages, 2=Current, 3=Combined, 4=Cycle between 1 and 2 |
| enable          | boolean   | true/false  | Enable/disable boost conversion |
| factory         | boolean   | true/false  | Return to factory defaults and restart |
| fan             | integer   | 0-255       | Override auto fan speed. 0=auto Fan, 1-255=Fan speed |
| mqtt_timeout    | integer   | 20-65535    | Restart if no Mqtt commands seen in this many secs |
| overtemp_thresh | integer   | 20-255      | Raise alarm and disable converter for 3 mins if this temperature is exceeded |
| restart         | boolean   | true/false  | Restart controller |
| save            | boolean   | true/false  | Save all parameters to Flash memory |
| volts           | float      | 8.0-120.0   | Output Voltage in V |

### Calibration Keywords

| Keyword         | Data Type | Comment                                  |
| :-------------: | :-------: | :--------------------------------------- |
| vin_bias        | float      | Measured Vin bias offset. Normally zero |
| vin_res         | float      | Measured Vin units per mV |
| vout_bias       | float      | Measured Vout bias offset. Normally zero |
| vout_res        | float      | Measured Vout units per mV |
| iout_bias       | float      | Measured Iout bias offset. Normally zero |
| iout_res        | float      | Measured Iout units per mV |
| pwm_vout_bias   | float      | Vout offset. Normally zero |
| pwm_vout_step   | float      | Steps per mV |


## Response Keywords

| Keyword         | Data Type | Range      | Comment                                    |
| :-------------: | :-------: | :---------- | :---------------------------------------- |
| bl_intensity    | integer   | 0-1023      | Backlight Intensity |
| display_mode    | integer   | 0-4         | Contents of TFT display. 0=blank, 1=Voltages, 2=Current, 3=Combined, 4=Cycle between 1 and 2 |
| enable          | boolean   | true/false  | Converter enable state |
| mqtt_timeout    | integer   | 20-65535    | Configyred MQTT Timeout |
| overtemp_thresh | integer   | 20-255      | Configured over temperature threshold |
| reset_reason    | integer   | 1-16        | ESP32 [reset reason]( https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/ResetReason/ResetReason.ino) |
| targetIout      | integer   | 1-15000     | Configured Constant Current threshold in mA |
| targetVout      | integer   | 1-120000    | Configured Output Voltage in mV |
| Vin             | integer   | 1-120000    | Last measured Vin in mV|
| Vin_smoothed    | integer   | 1-120000    | Rolling average of Vin |
| Vout            | integer   | 1-120000    | Last measured Vout in mV|
| Vin_smoothed    | integer   | 1-120000    | Rolling average of Vout |
| Iout            | integer   | 1-15000     | Last measured Iout in mA|
| Iout_smoothed   | integer   | 1-15000     | Rolling average of Iout |
| fan             | integer   | 0-255       | Current fan voltage |
| overtemp        | boolean   | true/false  | Temperature alarm state |
| temperature     | signedint | -127 to 127 | Current temperature |
   

