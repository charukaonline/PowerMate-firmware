#ifndef POWER_SENSOR_H
#define POWER_SENSOR_H

#include <Arduino.h>

class PowerSensor {
private:
    int rxPin, txPin;
    
public:
    PowerSensor(int rx, int tx);
    void begin();
    bool readValues(float &dcVoltage, float &dcCurrent, float &batteryVoltage, float &batteryCurrent, float &batteryPercentage);
    String getJSON(float dcVoltage, float dcCurrent, float batteryVoltage, float batteryCurrent, float batteryPercentage);

private:
    float calculateBatteryPercentage(float voltage);
};

#endif