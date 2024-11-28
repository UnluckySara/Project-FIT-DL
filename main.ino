#include <Wire.h>
#include <Temperature_LM75_Derived.h>
#include <Arduino_HTS221.h>
#include <SerialMenuCmd.h>
#include <TSL2571.h>
#include "i2c.h"
#include "i2c_LPS331.h"

/*======================================================================*/

TSL2571 tsl;
LPS331 lps331;
Generic_LM75 stlm75;

/*======================================================================*/

bool start_screen = true;
String inputBuffer = "";
float upper_temp_value = 30.0;
float lower_temp_value = -5.0;
float critical_pressure = 1000.0;
float critical_humidity = 90.0;
uint8_t CmdCode;
float pressure, temperature, humidity, illuminance = {};

/*======================================================================*/

SerialMenuCmd menu;

tMenuCmdTxt txt_welcome[] = "\nWelcome to the configuration window.\n";
tMenuCmdTxt txt_values[] = "s - display current settings";
tMenuCmdTxt txt_set_h[] = "h - set critical value humidity.";
tMenuCmdTxt txt_set_p[] = "p - set critical value pressure.";
tMenuCmdTxt txt_set_tl[] = "t - set lower critical temperature value.";
tMenuCmdTxt txt_set_th[] = "T - set higher critical temperature value.";
tMenuCmdTxt txt_reset[] = "r - reset device.";
tMenuCmdTxt txt_display[] = "d - display last measurement";

tMenuCmdTxt txt5_DisplayMenu[] = "? - Display menu";

tMenuCmdTxt txt_Prompt[] = "";

/*======================================================================*/

#define NbCmds sizeof(list) / sizeof(stMenuCmd)
#define LED_PIN 13

/*======================================================================*/
void(* resetFunc) (void) = 0;

void do_settings(void){
    Serial.println();
    Serial.println("Current settings:");
    Serial.print("Critical value humidity: ");
    Serial.print(critical_humidity);
    Serial.println(" %");
    Serial.print("Critical value pressure: ");
    Serial.print(critical_pressure);
    Serial.println(" mbar");
    Serial.print("Lower critical temperature value: ");
    Serial.print(lower_temp_value);
    Serial.println(" C");
    Serial.print("Upper critical temperature value: ");
    Serial.print(upper_temp_value);
    Serial.println(" C");

    menu.giveCmdPrompt();
}


void do_display(void)
{

   
    Serial.print("Humidity HTS221: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure LPS331: ");
    Serial.print(pressure);
    Serial.println(" mbar");

    Serial.print("Temperature STLM75: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Illuminance TSL2571: ");
    Serial.print(illuminance);
    Serial.println(" lux");

    menu.giveCmdPrompt();
}

void do_set_h(void){
  String aValue = "Enter critical humidity value";

  if (menu.getStrValue(aValue) == false)
  {

    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();

    critical_humidity = atof(aValue.c_str());
    Serial.print("Critical value humidity: ");
    Serial.print(critical_humidity);
    Serial.println(" %");
  }

  menu.giveCmdPrompt();

}

void do_set_p(void){
  String aValue = "Enter critical pressure value";

  if (menu.getStrValue(aValue) == false)
  {

    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();

    critical_pressure = atof(aValue.c_str());
    Serial.print("Critical pressure value: ");
    Serial.print(critical_pressure);
    Serial.println(" mbar");
  }

  menu.giveCmdPrompt();

}

void do_set_tl(void){
  String aValue = "Enter lower critical temperature value";

  if (menu.getStrValue(aValue) == false)
  {

    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();

    lower_temp_value = atof(aValue.c_str());
    Serial.print("Lower critical value temperature: ");
    Serial.print(lower_temp_value);
    Serial.println(" C");
  }

  if(temp_check()){
  menu.giveCmdPrompt();
  } else {
    do_set_tl();
  }

}

void do_set_th(void){
  String aValue = "Enter upper critical temperature value";

  if (menu.getStrValue(aValue) == false)
  {

    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();

    upper_temp_value = atof(aValue.c_str());
    Serial.print("Upper critical value temperature: ");
    Serial.print(upper_temp_value);
    Serial.println(" C");
  }
  
  if(temp_check()){
  menu.giveCmdPrompt();
  } else {
    do_set_th();
  }

}

bool temp_check(void){
  if(lower_temp_value > upper_temp_value){
    Serial.println();
    Serial.println("Error!!!");
    Serial.println();
    //Serial.println("Lower critical value must be lower than upper critical value!!");
    Serial.println("Values you are trying to set:");
    Serial.println();
    Serial.print("Lower critical value temperature: ");
    Serial.print(lower_temp_value);
    Serial.println(" C");
    Serial.print("Upper critical value temperature: ");
    Serial.print(upper_temp_value);
    Serial.println(" C");
    Serial.println();
    Serial.println("Try again.");
    return(false);
  }else{
    return(true);
  }
}

/*======================================================================*/

stMenuCmd list[] = {
    {txt_welcome},
    {txt_values, 's', do_settings},
    {txt_set_h, 'h' , do_set_h},
    {txt_set_p, 'p', do_set_p},
    {txt_set_tl, 't', do_set_tl},
    {txt_set_th, 'T', do_set_th},
    {txt_display, 'd', do_display},
    {txt_reset, 'r', [](){Serial.println();Serial.println("Resetting device...");
                                          delay(3000);
                                          resetFunc();}},

    {txt5_DisplayMenu, '?', []() { menu.ShowMenu();
                                   menu.giveCmdPrompt();}}
    };



/*======================================================================*/


/*======================================================================*/



void setup()
{
    pinMode(LED_PIN, OUTPUT);


    Serial.begin(9600); 
    Wire.begin();

    if (menu.begin(list, NbCmds, txt_Prompt) == false)
    {
      while (true);
    }
    


    if (lps331.initialize())
    {
      Serial.println("LPS found!");
    } 
    else
    {
        Serial.println("Sensor missing");
        while(1);
    }

    if (!HTS.begin()) 
    {
      Serial.println("Sensor missing");
      while (1);
    }else{
      Serial.println("HTS found!");
    }

    tsl.getAddr_TSL2571(TSL2571_DEFAULT_ADDRESS);
    tsl.begin();

    delay(1000);


    menu.ShowMenu();
    menu.giveCmdPrompt();



}
 
void loop()
{ 
  /*if(start_screen)
  {
    Title_Window();
  }
  else
  {*/
    //lps331.getTemperature(tempLPS);
    //float tempHTS = HTS.readTemperature();


    if(illuminance>0.1){
      digitalWrite(LED_PIN, HIGH);
    }else{
      digitalWrite(LED_PIN, LOW);
    }

    CmdCode = menu.UserRequest();

    if (CmdCode != 0)
    {
      menu.ExeCommand(CmdCode);
    }



    humidity = HTS.readHumidity();
    lps331.getMeasurement(pressure);
    temperature = stlm75.readTemperatureC();

    tsl.setUpALS();
    delay(500);
    tsl.Measure_ALS();
    illuminance = tsl.tsl_alsData.L;

  
    
    
    //Serial.print("Luminance TSL25721: ");
    //Serial.print(luminance);
    //Serial.println(" lux");
    
    //Serial.print("Temperature HTS221: ");
    //Serial.print(tempHTS);
    //Serial.println(" C");

    //Serial.print("Temperature LPS331: ");
    //Serial.print(tempLPS);
    //Serial.println(" C");


    /*Serial.println();
    if (Serial.available() > 0)
  {
    start_screen = true;
    Serial.println("Press twice on the space.");
  }
   delay(1000);
   start_screen=true;
  }*/
  
    
}




/*
void Title_Window()
{
   if (Serial.available() > 0) 
    { 
      
      String input = Serial.readStringUntil('\n'); 
      if (input.equals("?")||input.equals(" "))
      {    
          Serial.println();
          Serial.println("Welcome to the configuration window.");
          Serial.println();
          
          Serial.println("Current settings:");
          Serial.print("Critical value humidity: ");
          Serial.print(critical_humidity);
          Serial.println(" %");
          Serial.print("Critical value pressure: ");
          Serial.print(critical_pressure);
          Serial.println(" mbar");
          Serial.print("Lower critical temperature value: ");
          Serial.print(lower_temp_value);
          Serial.println(" C");
          Serial.print("Upper critical temperature value: ");
          Serial.print(upper_temp_value);
          Serial.println(" C");
          Serial.println();

          Serial.println("Options:");
          Serial.println("h <value> - set critical value humidity.");
          Serial.println("p <value> - set critical value pressure.");
          Serial.println("tl <value> - set lower critical temperature value.");
          Serial.println("th <value> - set upper critical temperature value.");
          Serial.println("reset - device reset.");
          Serial.println("start - do the measurement.");   
      }

      if (input.equals("start")) 
      {
        Serial.println();
        Serial.println("Starting sensor data display...");
        start_screen = false; 
      } 
      else if (input.startsWith("h ")) 
      {
        String valueStr = input.substring(2);
        critical_humidity = valueStr.toFloat();
        Serial.print("Set critical humidity to: ");
        Serial.print(critical_humidity);
        Serial.println(" %");
      }
      else if (input.startsWith("p ")) 
      {
         String valueStr = input.substring(2);
         critical_pressure = valueStr.toFloat();
         Serial.print("Set critical pressure to: ");
         Serial.print(critical_pressure);
         Serial.println(" mbar");
        } 
        else if (input.startsWith("tl "))
        { 
          String valueStr = input.substring(3);
          lower_temp_value = valueStr.toFloat();
          Serial.print("Set lower critical temperature to: ");
          Serial.print(lower_temp_value);
          Serial.println(" C");
        } 
        else if (input.startsWith("th "))
        { 
          String valueStr = input.substring(3);
          upper_temp_value = valueStr.toFloat();
          Serial.print("Set upper critical temperature to: ");
          Serial.print(upper_temp_value);
          Serial.println(" C");
        } 
        else if (input.equals("reset")) 
        {
            Serial.println("Resetting device...");
            delay(5000);
            resetFunc();
        }
        else 
        {
            Serial.println("Invalid command. Please try again.");
        }
      
    }
   
}*/



