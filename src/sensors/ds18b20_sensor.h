#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20_Sensor {
private:
    int pin;
    OneWire* oneWire;
    DallasTemperature* sensors;
    float lastValidTempC = -1000.0; // Initialize to an impossible value

public:
    DS18B20_Sensor(int dataPin);
    void begin();
    float getTemperatureC();
    float getTemperatureF();
    bool isReadingValid(float tempC); // Helper to check if reading is valid
};

#endif
