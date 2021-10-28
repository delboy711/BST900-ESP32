/************************************
 * Task to update TFT Screen
 * 
 * Note: platformio.ini file configures 
 * TFT parameters
 * **********************************/

#include "defines.h"
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
extern Mystate state;        //Global structure holding controller state
extern Mysettings settings;
extern TaskHandle_t Mqtt_Task;
extern IPAddress ip;
char charbuff[48];
uint8_t cyclecount=0;

enum TFT_mode {         //Display modes
      off,
      voltage,
      current,
      combined,
      cycle
};

void backlight(uint16_t brightness) {
      ledcWrite(5, brightness);
}

void disp_voltage() {
#ifdef TEMP             //Temp reading
 //     tft.setCursor(200, 15, 2); tft.setTextSize(1);  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
 //     tft.print(state.temperature); tft.print("°C");
#endif
      if (state.cc_smoothed) tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
      else tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.setCursor(205, 0, 2); tft.setTextSize(1);  
      tft.print(" CC ");

      tft.setCursor(0, 0, 4);  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.setTextSize(2); tft.print("Vin "); tft.setTextFont(6);
      tft.setTextSize(1); sprintf(charbuff, "%4.1f  ", state.Vin_smoothed/1000.0); tft.print(charbuff);tft.setTextFont(4);

      tft.setCursor(0, 46, 4);   tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK); tft.setTextSize(2); tft.print("Vout "); tft.setTextFont(6);
      tft.setTextSize(1); sprintf(charbuff, (state.Vout_smoothed < 100000) ? "%5.2f  " : "%5.1f  ", state.Vout_smoothed/1000.0); tft.print(charbuff);

      if(settings.enable) {         //Show Vset when enabled, and "Disabled" when not
      tft.setCursor(0, 92, 4);   tft.setTextColor(TFT_SKYBLUE, TFT_BLACK); tft.setTextSize(2); tft.print("Vset "); tft.setTextFont(6);
      tft.setTextSize(1); sprintf(charbuff, (settings.Vout < 100000) ? "%5.2f  " : "%5.1f  ",(float)settings.Vout/1000.0); tft.print(charbuff);
      } else tft.setCursor(0, 92, 4);   tft.setTextColor(TFT_SKYBLUE, TFT_BLACK); tft.setTextSize(2); tft.print("Disabled");
}

void disp_current() {
#ifdef TEMP             //Temp reading
      tft.setCursor(132, 0, 2); tft.setTextSize(2);  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.print(state.temperature); tft.print(" C  ");
#endif
      if (state.cc_smoothed) tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
      else tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.setCursor(205, 0, 2); tft.setTextSize(1);  
      tft.print(" CC ");

      tft.setCursor(0, 46, 4);
      if (state.cc_smoothed) tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      else tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.setTextSize(2); tft.print("Iout "); tft.setTextFont(6); tft.setTextSize(1); sprintf(charbuff, "%5.2f ", state.Iout_smoothed/1000.0); tft.print(charbuff);
      if(settings.enable) {         //Show Iset when enabled, and "Disabled" when not
      tft.setCursor(0, 92, 4);    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);  tft.setTextSize(2); tft.print("Iset ");
      tft.setTextFont(6); tft.setTextSize(1); sprintf(charbuff, "%5.2f ",(float)settings.Iout/1000.0); tft.print(charbuff); 
      } else tft.setCursor(0, 92, 4);   tft.setTextColor(TFT_SKYBLUE, TFT_BLACK); tft.setTextSize(2); tft.print("Disabled");

}

void disp_combined() {
#ifdef TEMP             //Temp reading
      tft.setCursor(132, 0, 2); tft.setTextSize(2);  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.print(state.temperature); tft.print(" C  ");
#endif
      if (state.cc_smoothed) tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
      else tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.setCursor(205, 0, 2); tft.setTextSize(1);  
      tft.print(" CC ");

      tft.setCursor(0, 0, 2);tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.setTextSize(2); tft.print("Vi "); 
      sprintf(charbuff, "%5.1f ", state.Vin_smoothed/1000.0); tft.print(charbuff); 

      tft.setCursor(0, 46, 2);  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);  tft.print("Vo ");
      sprintf(charbuff, "%5.1f  ", state.Vout_smoothed/1000.0); tft.print(charbuff);

      tft.setCursor(132, 46, 2);  tft.setTextColor(TFT_BLUE, TFT_BLACK); tft.print("Io "); sprintf(charbuff, " %4.1f ", state.Iout_smoothed/1000.0); tft.print(charbuff);

      if(settings.enable) {         //Show Vset when enabled, and "Disabled" when not
      tft.setCursor(0, 92, 2);  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK); tft.print("Vs ");
      sprintf(charbuff, "%5.1f  ", (float)settings.Vout/1000.0); tft.print(charbuff); 
      } else tft.setCursor(0, 92, 2);  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK); tft.print("Disabled");

      tft.setCursor(132, 92, 2); tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.print("Is "); sprintf(charbuff, " %4.1f ", settings.Iout/1000.0); tft.print(charbuff); 
}


void disp_cycle() {
      tft.fillScreen(TFT_BLACK);
      if(cyclecount >0) {
            disp_current();
            cyclecount=0;
      } else {
            disp_voltage();
            cyclecount=1;
      } 

}



void Tft_Task_function( void * parameter) {

      uint8_t last_disp = settings.display_mode;   //Last screen shown
      //Setup TFT Display
      tft.init();
      tft.setRotation(1);               //Landscape mode
      tft.fillScreen(TFT_BLACK);
      ledcSetup(5, 2000, 11);          //Create a PWM channel for Back light
      ledcAttachPin(TFT_BL, 5);        //Enable Back light PWM
      backlight(settings.bl_intensity);
      Serial.println("Welcome");
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.setCursor(10, 4, 4);
      sprintf(charbuff, "Welcome to %s", HOSTNAME); tft.println(charbuff);
      tft.setCursor(10, 46, 4);
      sprintf(charbuff, "Connecting"); tft.println(charbuff);
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);            //Wait for IP address
      tft.setCursor(10, 46, 4);
      sprintf(charbuff, "IP  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]); tft.println(charbuff);
      delay(6000);
      tft.fillScreen(TFT_BLACK);
      xTaskNotify(Mqtt_Task, 0x00, eNotifyAction::eNoAction);      //Kick off mqtt


      for(;;) {

            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));     //Wait for up to 2 sec for a wake up, and then proceed anyway
            /******* 
             * To wake task use
             *  xTaskNotify(Tft_Task, 0x00, eNotifyAction::eNoAction);  command from another function
             */
            backlight(settings.bl_intensity);
            if (settings.display_mode != last_disp)  tft.fillScreen(TFT_BLACK);  //Wipe screen if changing modes
            switch (settings.display_mode) {
                  case off:
                        backlight(0);
                  break;
                  case voltage:
                        disp_voltage();
                  break;
                  case current:
                        disp_current();
                  break;
                  case combined:
                        disp_combined();
                  break;
                  case cycle:
                        disp_cycle();
                  break;
                  default:                //No action
                  break;
            }
            last_disp = settings.display_mode;

      }

      vTaskDelete( NULL );
}