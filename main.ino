#include <Wire.h>
#include <Temperature_LM75_Derived.h>
#include <Arduino_HTS221.h>
#include <SerialMenuCmd.h>
#include <TSL2571.h>
#include "i2c.h"
#include "i2c_LPS331.h"
#include "RTClib.h"

/*======================================================================*/

TSL2571 tsl;
LPS331 lps331;
Generic_LM75 stlm75;
RTC_DS1307 rtc;

/*======================================================================*/

bool continous_display = false;
float upper_temp_value = 30.0;
float lower_temp_value = -5.0;
float critical_pressure = 1000.0;
float critical_humidity = 90.0;
uint8_t CmdCode;
float pressure, temperature, humidity, illuminance = {};


unsigned long prev_time = 0;

/*======================================================================*/

SerialMenuCmd menu;

tMenuCmdTxt txt_welcome[] = "\nWelcome to the configuration window.\n";
tMenuCmdTxt txt_values[] = "s - display current settings.";
tMenuCmdTxt txt_set_h[] = "h - set critical value humidity.";
tMenuCmdTxt txt_set_p[] = "p - set critical value pressure.";
tMenuCmdTxt txt_set_tl[] = "t - set lower critical temperature value.";
tMenuCmdTxt txt_set_th[] = "T - set higher critical temperature value.";
tMenuCmdTxt txt_reset[] = "r - reset device.";
tMenuCmdTxt txt_display[] = "m - display last measurement.";
tMenuCmdTxt txt_set_date[] = "d - set current date";
tMenuCmdTxt txt_continous[] = "c - toggle continous measurement display.";

tMenuCmdTxt txt5_DisplayMenu[] = "? - Display menu.";

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
    Serial.println();

    char time_array[20];
    sprintf(time_array,"%02d:%02d:%02d %02d,%02d,%04d",rtc.now().hour(),rtc.now().minute(),rtc.now().second(),rtc.now().day(),rtc.now().month(),rtc.now().year());
    Serial.println(time_array);
    
   
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
    Serial.print(illuminance,4);
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


bool isValidDate(int year, int month, int day, int hours, int minutes, int seconds) {
    if (month < 1 || month > 12) return false;

    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[1] = 29; 
    }

    if (day < 1 || day > daysInMonth[month - 1]) return false;

    if (hours < 0 || hours > 23) return false;
    if (minutes < 0 || minutes > 59) return false;
    if (seconds < 0 || seconds > 59) return false;

    return true;
}

void do_set_date(void) {
    String aValue;
    int tempYear, tempMonth, tempDay, tempHours, tempMinutes, tempSeconds;
    byte seconds, minutes, hours, day, month;
    uint16_t days, year;


    aValue = "Enter year:";
    if (menu.getStrValue(aValue)) {
        tempYear = atoi(aValue.c_str());
    } else {
        Serial.println("Invalid entry for year.");
        menu.giveCmdPrompt();
        return;
    }

    aValue = "Enter month:";
    if (menu.getStrValue(aValue)) {
        tempMonth = atoi(aValue.c_str());
    } else {
        Serial.println("Invalid entry for month.");
        menu.giveCmdPrompt();
        return;
    }

    aValue = "Enter day:";
    if (menu.getStrValue(aValue)) {
        tempDay = atoi(aValue.c_str());
    } else {
        Serial.println("Invalid entry for day.");
        menu.giveCmdPrompt();
        return;
    }

    aValue = "Enter hour:";
    if (menu.getStrValue(aValue)) {
        tempHours = atoi(aValue.c_str());
    } else {
        Serial.println("Invalid entry for hour.");
        menu.giveCmdPrompt();
        return;
    }

    aValue = "Enter minute:";
    if (menu.getStrValue(aValue)) {
        tempMinutes = atoi(aValue.c_str());
    } else {
        Serial.println("Invalid entry for minute.");
        menu.giveCmdPrompt();
        return;
    }

    aValue = "Enter second:";
    if (menu.getStrValue(aValue)) {
        tempSeconds = atoi(aValue.c_str());
    } else {
        Serial.println("Invalid entry for second.");
        menu.giveCmdPrompt();
        return;
    }

    if (!isValidDate(tempYear, tempMonth, tempDay, tempHours, tempMinutes, tempSeconds)) {
        Serial.println("\nInvalid date or time entered. Please try again.");
        menu.giveCmdPrompt();
        return;
    }

    year = tempYear;
    month = tempMonth;
    day = tempDay;
    hours = tempHours;
    minutes = tempMinutes;
    seconds = tempSeconds;
    

    switch (month) {
        case 1: days = day; break;
        case 2: days = day + 31; break;
        case 3: days = day + 59; break;
        case 4: days = day + 90; break;
        case 5: days = day + 120; break;
        case 6: days = day + 151; break;
        case 7: days = day + 181; break;
        case 8: days = day + 212; break;
        case 9: days = day + 243; break;
        case 10: days = day + 273; break;
        case 11: days = day + 304; break;
        case 12: days = day + 334; break;
        default: break;
    }
    prev_time = millis();
    Serial.println("\nTime set successfully:");

    rtc.adjust(DateTime(year,month,day,hours,minutes,seconds));
    char time_array[20];
    sprintf(time_array, "%02d:%02d:%02d %02d,%02d,%04d", hours, minutes, seconds, day, month, year);
    Serial.println(time_array);

    menu.giveCmdPrompt();
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

/*void do_time(void) {
    if (millis() - prev_time >= 1000) {
        prev_time = millis();
        seconds++;

        if (seconds == 60) {
            seconds = 0;
            minutes++;
            if (minutes == 60) {
                minutes = 0;
                hours++;
                if (hours == 24) {
                    hours = 0;
                    day++;

                    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

                    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
                        daysInMonth[1] = 29; 
                    }

                    if (day > daysInMonth[month - 1]) {
                        day = 1;
                        month++;

                        if (month > 12) {
                            month = 1;
                            year++;
                        }
                    }
                }
            }
        }
    }
}*/

/*======================================================================*/

stMenuCmd list[] = {
    {txt_welcome},
    {txt_values, 's', do_settings},
    {txt_set_h, 'h' , do_set_h},
    {txt_set_p, 'p', do_set_p},
    {txt_set_tl, 't', do_set_tl},
    {txt_set_th, 'T', do_set_th},
    {txt_set_date, 'd',do_set_date},
    {txt_display, 'm', do_display},
    {txt_continous, 'c', [](){continous_display=(!continous_display);}},
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

    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1);
    }

    if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
      do_set_date();
    }

    tsl.getAddr_TSL2571(TSL2571_DEFAULT_ADDRESS);
    tsl.begin();

    delay(1000);


    menu.ShowMenu();
    menu.giveCmdPrompt();



}
 
void loop()
{ 
    tsl.setUpALS();

    if(illuminance>0.01){
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
  
     //do_time();
  
     if(continous_display)
     {
        do_display();
     }
      
      
     tsl.Measure_ALS();
     illuminance = tsl.tsl_alsData.L;
    
    
}