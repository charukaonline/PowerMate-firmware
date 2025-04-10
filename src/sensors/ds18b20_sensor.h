#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20_Sensor {
private:
    int pin;
    OneWire* oneWire;
    DallasTemperature* sensors;

public:
    DS18B20_Sensor(int dataPin);
    void begin();
    float getTemperatureC();
    float getTemperatureF();
};

#endif
