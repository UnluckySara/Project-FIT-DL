#include <Wire.h>
#include <Temperature_LM75_Derived.h>
#include <Arduino_HTS221.h>
#include <SerialMenuCmd.h>
#include <TSL2571.h>
#include "i2c.h"
#include "i2c_LPS331.h"
#include "RTClib.h"
/*======================================================================*/

// Define LED pins and command count
#define NbCmds sizeof(list) / sizeof(stMenuCmd)
#define LEDR_PIN 13
#define LEDG_PIN 12

// EEPROM CONFIG 
#define EEPROM_I2C_ADDRESS 0x50
#define EEPROM_WRITE_DELAY 5
#define EEPROM_START_ADDRESS 0x0001 
#define EEPROM_SENSOR_START_ADDRESS 0x16  
#define EEPROM_SENSOR_END_ADDRESS 0x7D00


/*======================================================================*/

// Sensor and peripheral objects
TSL2571 tsl;
LPS331 lps331;
Generic_LM75 stlm75;
RTC_DS1307 rtc;

/*======================================================================*/

// Configuration settings
uint8_t CmdCode;
unsigned long prev_millis = 0;
float pressure, temperature, humidity, illuminance = {};

bool continous_display = false;
bool LEDR_STATE= LOW;
bool LEDG_STATE= LOW;

struct ConfigData {
  float upper_temp_value;
  float lower_temp_value;
  float critical_pressure;
  float critical_humidity;
  bool alert;
};

ConfigData config = {
  .upper_temp_value = 30.0,
  .lower_temp_value = -5.0,
  .critical_pressure = 1000.0,
  .critical_humidity = 90.0,
  .alert = false
};

/*======================================================================*/

// Menu configuration
SerialMenuCmd menu;

tMenuCmdTxt txt_welcome[] = "Welcome to the configuration window.\n";
tMenuCmdTxt txt_values[] = "s - display current settings.";
tMenuCmdTxt txt_set_h[] = "h - set critical value humidity.";
tMenuCmdTxt txt_set_p[] = "p - set critical value pressure.";
tMenuCmdTxt txt_set_tl[] = "t - set lower critical temperature value.";
tMenuCmdTxt txt_set_th[] = "T - set higher critical temperature value.";
tMenuCmdTxt txt_reset[] = "r - reset device.";
tMenuCmdTxt txt_display[] = "m - display last measurement.";
tMenuCmdTxt txt_set_date[] = "d - set current date.";
tMenuCmdTxt txt_continous[] = "c - toggle continous measurement display.";
tMenuCmdTxt txt_clear_alert[] = "a - clear alert.";
tMenuCmdTxt txt_eeprom_save[] = "e - save configuration to EEPROM.";
tMenuCmdTxt txt_eeprom_load[] = "E - load configuration from EEPROM.";
tMenuCmdTxt txt_eeprom_load_data[] = "D - load data from EEPROM.";
tMenuCmdTxt txt5_DisplayMenu[] = "? - Display menu.";

tMenuCmdTxt txt_Prompt[] = "";

/*======================================================================*/

// Function pointer for reset
void(* resetFunc) (void) = 0;

/*======================================================================*/
void saveSensorDataToEEPROM(float temp, float humidity, float pressure, float illuminance, uint16_t startAddress, uint16_t endAddress) 
{
  uint16_t currentAddress = startAddress;
  
  while (!isAddressEmpty(currentAddress)) 
  {
    currentAddress += sizeof(float) * 4;
    if (currentAddress >= endAddress) 
    {
      currentAddress = startAddress;
    }
  }

  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write(highByte(currentAddress));
  Wire.write(lowByte(currentAddress));

  byte *data = (byte*)&temp;
  for (int i = 0; i < sizeof(float); i++) Wire.write(data[i]);
  data = (byte*)&humidity;
  for (int i = 0; i < sizeof(float); i++) Wire.write(data[i]);
  data = (byte*)&pressure;
  for (int i = 0; i < sizeof(float); i++) Wire.write(data[i]);
  data = (byte*)&illuminance;
  for (int i = 0; i < sizeof(float); i++) Wire.write(data[i]);

  if (Wire.endTransmission() != 0) 
  {
    Serial.println("Failed to write sensor data to EEPROM!");
    return;
  }

  delay(EEPROM_WRITE_DELAY);
  Serial.println("Sensor data saved to EEPROM!");
}

void readSensorDataFromEEPROM(uint16_t startAddress) 
{
  Serial.println();
  Serial.println("Reading sensor data from EEPROM...");
  uint16_t currentAddress = startAddress;

  while (currentAddress < EEPROM_SENSOR_END_ADDRESS) 
  {
    if (isAddressEmpty(currentAddress)) 
    {
      Serial.println("End of data found.");
      break;
    }

    float temp, humidity, pressure, illuminance;
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write(highByte(currentAddress));
    Wire.write(lowByte(currentAddress));
    Wire.endTransmission();

    Wire.requestFrom(EEPROM_I2C_ADDRESS, sizeof(float) * 4);
    if (Wire.available() == sizeof(float) * 4) 
    {
      byte *data = (byte*)&temp;
      for (int i = 0; i < sizeof(float); i++) data[i] = Wire.read();
      data = (byte*)&humidity;
      for (int i = 0; i < sizeof(float); i++) data[i] = Wire.read();
      data = (byte*)&pressure;
      for (int i = 0; i < sizeof(float); i++) data[i] = Wire.read();
      data = (byte*)&illuminance;
      for (int i = 0; i < sizeof(float); i++) data[i] = Wire.read();

      Serial.println();      
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
  
      Serial.print("Pressure: ");
      Serial.print(pressure);
      Serial.println(" mbar");
  
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.println(" C");
  
      Serial.print("Illuminance: ");
      Serial.print(illuminance);
      Serial.println(" lux");

      currentAddress += sizeof(float) * 4;
    } 
    else 
    {
      Serial.println("Error reading data from EEPROM");
      break;
    }
  }
}

bool isAddressEmpty(uint16_t address) 
{
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write(highByte(address));
  Wire.write(lowByte(address));
  Wire.endTransmission();

  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);
  return Wire.available() && Wire.read() == 0xFF;
}

/*======================================================================*/

//EEPROME support functions

void saveConfigToEEPROM() 
{
  Serial.println();
  Serial.println("Saving configuration to EEPROM...");
  
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write(highByte(EEPROM_START_ADDRESS)); 
  Wire.write(lowByte(EEPROM_START_ADDRESS)); 

  byte* dataPtr = (byte*)&config;
  for (int i = 0; i < sizeof(ConfigData); i++) 
  {
    Wire.write(dataPtr[i]);
  }

  byte status = Wire.endTransmission();
  if (status != 0) 
  {
    Serial.println("Failed to write to EEPROM!");
    return;
  }

  delay(EEPROM_WRITE_DELAY);
  Serial.println("Configuration saved successfully!");
}

void loadConfigFromEEPROM() 
{
  Serial.println();
  Serial.println("Loading configuration from EEPROM...");

  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write(highByte(EEPROM_START_ADDRESS)); 
  Wire.write(lowByte(EEPROM_START_ADDRESS));
  Wire.endTransmission();

  Wire.requestFrom(EEPROM_I2C_ADDRESS, sizeof(ConfigData));
  if (Wire.available() == sizeof(ConfigData)) 
  {
    byte* dataPtr = (byte*)&config;
    for (int i = 0; i < sizeof(ConfigData); i++) 
    {
      dataPtr[i] = Wire.read();
    }
    Serial.println("Configuration loaded successfully!");
  } else {
    Serial.println("Failed to read from EEPROM!");
  }  
  Serial.print("Critical Humidity: ");
  Serial.println(config.critical_humidity);
  Serial.print("Critical Pressure: ");
  Serial.println(config.critical_pressure); 
  Serial.print("Lower Temperature Value: ");
  Serial.println(config.lower_temp_value);
  Serial.print("Upper Temperature Value: ");
  Serial.println(config.upper_temp_value);
}

/*======================================================================*/
// Functions to handle menu commands and functionality

// Display current settings
void do_settings(void){
    Serial.println();
    Serial.println("Current settings:");
    Serial.print("Critical value humidity: ");
    Serial.print(config.critical_humidity);
    Serial.println(" %");
    Serial.print("Critical value pressure: ");
    Serial.print(config.critical_pressure);
    Serial.println(" mbar");
    Serial.print("Lower critical temperature value: ");
    Serial.print(config.lower_temp_value);
    Serial.println(" C");
    Serial.print("Upper critical temperature value: ");
    Serial.print(config.upper_temp_value);
    Serial.println(" C");

    menu.giveCmdPrompt();
}

// Display measurements
void do_display(void)
{
    Serial.println();
    char time_array[20];
    sprintf(time_array,"%02d:%02d:%02d %02d,%02d,%04d",rtc.now().hour(),rtc.now().minute(),rtc.now().second(),rtc.now().day(),rtc.now().month(),rtc.now().year());
    Serial.println(time_array);
    
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" mbar");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Illuminance: ");
    Serial.print(illuminance,4);
    Serial.println(" lux");


    saveSensorDataToEEPROM(temperature, humidity, pressure, illuminance, EEPROM_SENSOR_START_ADDRESS, EEPROM_SENSOR_END_ADDRESS);
    
    menu.giveCmdPrompt();
}

// Update critical humidity value
void do_set_h(void){
  String aValue = "Enter critical humidity value";

  if (menu.getStrValue(aValue) == false)
  {
    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();
    config.critical_humidity = atof(aValue.c_str());
    Serial.print("Critical value humidity: ");
    Serial.print(config.critical_humidity);
    Serial.println(" %");
  }

  menu.giveCmdPrompt();
}

// Update critical pressure value
void do_set_p(void){
  String aValue = "Enter critical pressure value";

  if (menu.getStrValue(aValue) == false)
  {
    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();
    config.critical_pressure = atof(aValue.c_str());
    Serial.print("Critical pressure value: ");
    Serial.print(config.critical_pressure);
    Serial.println(" mbar");
  }

  menu.giveCmdPrompt();
}

// Update lower critical temperature value
void do_set_tl(void){
  String aValue = "Enter lower critical temperature value";

  if (menu.getStrValue(aValue) == false)
  {
    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();
    config.lower_temp_value = atof(aValue.c_str());
    Serial.print("Lower critical value temperature: ");
    Serial.print(config.lower_temp_value);
    Serial.println(" C");
  }

  if(temp_check()){
  menu.giveCmdPrompt();
  } else {
    do_set_tl();
  }

}

// Update upper critical temperature value
void do_set_th(void){
  String aValue = "Enter upper critical temperature value";

  if (menu.getStrValue(aValue) == false)
  {
    Serial.println("Entry failure");
  }
  else
  {
    Serial.println();
    config.upper_temp_value = atof(aValue.c_str());
    Serial.print("Upper critical value temperature: ");
    Serial.print(config.upper_temp_value);
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

// Validate and set date
void do_set_date(void) {
    String aValue;
    int tempYear, tempMonth, tempDay, tempHours, tempMinutes, tempSeconds;
    byte seconds, minutes, hours, day, month;
    uint16_t year;

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
    
    rtc.adjust(DateTime(year,month,day,hours,minutes,seconds));
    char time_array[20];
    Serial.println();
    sprintf(time_array, "%02d:%02d:%02d %02d,%02d,%04d", hours, minutes, seconds, day, month, year);
    Serial.println(time_array);

    menu.giveCmdPrompt();
}

// Check if temperature values are valid
bool temp_check(void){
  if(config.lower_temp_value > config.upper_temp_value){
    Serial.println();
    Serial.println("Error!!!");
    Serial.println();
    //Serial.println("Lower critical value must be lower than upper critical value!!");
    Serial.println("Values you are trying to set:");
    Serial.println();
    Serial.print("Lower critical value temperature: ");
    Serial.print(config.lower_temp_value);
    Serial.println(" C");
    Serial.print("Upper critical value temperature: ");
    Serial.print(config.upper_temp_value);
    Serial.println(" C");
    Serial.println();
    Serial.println("Try again.");
    return(false);
  }else{
    return(true);
  }
}


// Menu command structure
stMenuCmd list[] = 
{
    {txt_welcome},
    {txt_values, 's', do_settings},
    {txt_set_h, 'h' , do_set_h},
    {txt_set_p, 'p', do_set_p},
    {txt_set_tl, 't', do_set_tl},
    {txt_set_th, 'T', do_set_th},
    {txt_set_date, 'd',do_set_date},
    {txt_display, 'm', do_display},
    {txt_eeprom_save, 'e', saveConfigToEEPROM},
    {txt_eeprom_load, 'E', loadConfigFromEEPROM},
    {txt_eeprom_load_data, 'D', []() { readSensorDataFromEEPROM(EEPROM_SENSOR_START_ADDRESS); }},
    {txt_continous, 'c', [](){continous_display=(!continous_display);}},
    {txt_reset, 'r', [](){Serial.println();
                          Serial.println("Resetting device...");
                          delay(3000);
                          resetFunc();}},

    {txt5_DisplayMenu, '?', []() { menu.ShowMenu();
                                   menu.giveCmdPrompt();}},
     {txt_clear_alert,'a',[](){config.alert=0;
                              menu.giveCmdPrompt();}}
    
   };
/*======================================================================*/

// Setup and initialization
void setup()
{
    pinMode(LEDR_PIN, OUTPUT);
    pinMode(LEDG_PIN, OUTPUT);

    Serial.begin(115200); 
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
        Serial.println("LPS missing");
        while(1);
    }

    if (!HTS.begin()) 
    {
      Serial.println("HTS missing");
      while (1);
    }else{
      Serial.println("HTS found!");
    }

    while (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    delay(10);
    } 
      Serial.println("RTC found!");
        

    if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
      do_set_date();
    }


    tsl.getAddr_TSL2571(TSL2571_DEFAULT_ADDRESS);
    loadConfigFromEEPROM();

    tsl.begin();

    delay(1000);
    
    menu.ShowMenu();
    menu.giveCmdPrompt();
}

// Main loop
void loop()
{ 
    tsl.setUpALS();
    unsigned long curr_millis = millis();
    int interval = 1000;

    if(config.alert){
      if(curr_millis - prev_millis > interval){
        LEDR_STATE=!LEDR_STATE;
      }
    }else if(illuminance>0.01){
      LEDR_STATE=HIGH;
    }else{
      LEDR_STATE=LOW;
    }

    if(curr_millis - prev_millis > interval){
        LEDG_STATE=!LEDG_STATE;
        prev_millis = curr_millis;
    }

    digitalWrite(LEDG_PIN, LEDG_STATE);
    digitalWrite(LEDR_PIN, LEDR_STATE);

    CmdCode = menu.UserRequest();

    if (CmdCode != 0)
    {
      menu.ExeCommand(CmdCode);
    }
 
    humidity = HTS.readHumidity();
    lps331.getMeasurement(pressure);
    temperature = stlm75.readTemperatureC();
    tsl.Measure_ALS();
    illuminance = tsl.tsl_alsData.L;

    if((temperature>config.upper_temp_value)||(temperature<config.lower_temp_value)||(pressure>config.critical_pressure)||(humidity>config.critical_humidity)){
      config.alert=1;
    }
    
    if(continous_display)
    {
      Serial.write(12);
      do_display();
        
    }    
}
