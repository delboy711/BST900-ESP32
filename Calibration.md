# Calibration
The default calibration should be fairly accurate, but if you find you need to recalibrate use this procedure.
Calibration of BST900-ESP32 is relatively easy. Equipment required is a multimeter, and a dummy resistive load
 such as an incandescent light bulb. Also required is a computer running an MQTT server such as [mosquitto](https://mosquitto.org/)
 with a text based MQTT client so commands can be sent to the BST900.
 
 To start calibrating first ensure the BST900 is connected to WiFi. On power on the BST900 will display its IP address on the TFT
  screen for a few seconds. If you do not see this ensure the code has been compiled with the correct credentials in the file 
  inc/credentials.h. Also ensure that credentials.h contains the correct credentials for your MQTT server, and that inc/defines.h contains 
  the IP address of your MQTT server. By default BST900 will subscribe to the topic 'bst900' to receive commands, and will send
   statistics every 30 seconds on the topic 'emon/bst900'.  Be aware that by default BST900 will expect to see a command at least every 
    two minutes on the 'bst900' topic or else it will declare a communication alarm and restart itself. This timeout period can 
    be adjusted by sending a JSON formatted MQTT command in the format.
    <code>{"mqtt_timeout":600}</code>
If using mosquitto as an MQTT client the command would look like this:-
<code>mosquitto_pub  -u 'mqttuser' -P 'mqttpassword' -t 'bst900' -m '{"mqtt_timeout":600}'</code>
  
 
 ## Calibrating Voltage readings
 With a stable power supply of at least 9V connected to the BST900, and the output terminals of the BST900
  left open circuit open a terminal window and subscribe to the topic BST900 is sending statistics on.
  <code>mosquitto_sub -v -u 'mqttuser' -P 'mqttpassword' -t 'emon/bst900'</code>
  On power up BST900 will send two messages giving the [reset reason](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/ResetReason/ResetReason.ino), setup parameters, and 
  the calibration parameters. All these parameters can be configured with MQTT commands.
  <code>
  
  
  </code>
  
  ###Calibrating Measuring Input Voltage
  With a multimeter attached to the input terminals compare the measered voltage with the reported Voltage 'Vin' in the statistics report.
  Adjust the parameter 'vin_res' until the reported Voltage matches the measured using MQTT commands in the format.
  <code>mosquitto_pub  -u 'mqttuser' -P 'mqttpassword' -t 'bst900' -m '{"vin_res":34.335}'</code>
  Now repeat the procedure using a much higher input voltage.
  Note: If you cannot find a value for 'vin_res' which works across all voltage ranges it is possible (but not likely) that adjusting the 
  parameter 'vin_bias' may help.
  
  ###Calibrating Measuring Output Voltage
  Now move the multmeter to the output terminals of the BST900. By definition the input voltage of a boost converter will appear on the output terminals even when it is not enabled.
  Repeat the previous procedure this time adjusting the parameter 'vout_res'.
  
  ###Calibrating Measuring Output Current
  Next with output terminals still open circuit look at the reported statistic for Iout and adjust the parameter 'iout_bias' until the reported current is zero.
  Now attach your dummy load to the output terminals. With your multimeter measure the current flowing in the dummy load.
  Compare with the reported value of Iout and adjust the parameter 'iout_res' until they match.  Repeat with another dummy load drawing a different current.
  
  
  ###Calibrating Output Voltage
  Calibration of output Voltage is semi automatic. So long as the parameter 'pwm_vout_step' is roughly accurate, then the BST900 will
   automatically adjust the output voltage until the measured output voltage matches the target voltage. If you find the output voltage
    is not accurate it may be adjusted by the parameter 'pwm_vout_step'. (Make small changes only).
    
  To set output voltages use the command:-
  <code>mosquitto_pub  -u 'mqttuser' -P 'mqttpassword' -t 'bst900' -m '{"volts":3500, "enable":true}'</code>
  Note: Voltage is expressed in milliVolts, and current is expressed in milliAmps as integer numbers. Enable is a boolean and is either 
  'true' or 'false'.
  Note: Changes to 'pwm_vout_step' will not take effect unless the output voltage is also changed.
  Note: Output voltage will drop if the converter is in 'constant current' mode. Ensure the constant current threshold is high enough when
   calibrating output voltage.
  
  ###Calibrating Output Constant Current Threshold.
  Calibration of the constant current threshold is fully automatic. Although there is a parameter 'pwm_iout_step' it has little effect and should not be adjusted.
  So long as the measured Iout is accurate then the constant current threshold should also be accurate.
  The command to set a constant current threshold would look like :-
  
  <code>mosquitto_pub  -u 'mqttuser' -P 'mqttpassword' -t 'bst900' -m '{"volts":3500, "amps":1200 "enable":true}'</code>
  
