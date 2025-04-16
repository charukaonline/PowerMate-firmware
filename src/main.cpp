#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is connected to GPIO4 (can be changed)
#define ONE_WIRE_BUS 14

// Setup oneWire and DallasTemperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup()
{
    Serial.begin(115200);
    sensors.begin();
}

void loop()
{
    sensors.requestTemperatures();            // Send the command to get temperatures
    float tempC = sensors.getTempCByIndex(0); // Get temperature of first sensor

    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println(" °C");

    delay(1000);
}
