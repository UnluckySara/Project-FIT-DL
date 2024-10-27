
#include <Wire.h>
#include <Temperature_LM75_Derived.h>
#include <Arduino_HTS221.h>
#include "i2c.h"
#include "i2c_LPS331.h"



LPS331 lps331;
Generic_LM75 temperature;


void setup()
{
    Serial.begin(9600);
    Wire.begin();

    


    if (lps331.initialize()){
      Serial.println("Sensor found!");
    } else
    {
        Serial.println("Sensor missing");
        while(1);
    }


    if (!HTS.begin()) {
      Serial.println("Sensor missing");
    while (1);
    }

    

}

void loop()
{
   

    float pressure, tempLPS;

    float tempHTS = HTS.readTemperature();
    float humidity    = HTS.readHumidity();


    lps331.getMeasurement(pressure);
    lps331.getTemperature(tempLPS);
    float tempSTLM = temperature.readTemperatureC();

    Serial.print("Humidity HTS221: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure LPS331: ");
    Serial.print(pressure);
    Serial.println(" mbar");

    Serial.print("Temperature LPS331: ");
    Serial.print(tempLPS);
    Serial.println(" C");

    Serial.print("Temperature STLM75: ");
    Serial.print(tempSTLM);
    Serial.println(" C");

    Serial.print("Temperature HTS221: ");
    Serial.print(tempHTS);
    Serial.println(" C");


   // Serial.print("Luminance TSL25721: ");
   // Serial.print(luminance);
   // Serial.println(" lux");

    Serial.println();

    delay(1000);
}

  