#ifndef POWER_SENSOR_H
#define POWER_SENSOR_H

#include <Arduino.h>

class PowerSensor {
private:
    int rxPin, txPin;
    
    // Last valid readings
    float lastDcVoltage = 0;
    float lastDcCurrent = 0;
    float lastBatteryVoltage = 0;
    float lastBatteryCurrent = 0;
    float lastBatteryPercentage = 0;
    
public:
    PowerSensor(int rx, int tx);
    void begin();
    bool readValues(float &dcVoltage, float &dcCurrent, float &batteryVoltage, float &batteryCurrent, float &batteryPercentage);
    String getJSON(float dcVoltage, float dcCurrent, float batteryVoltage, float batteryCurrent, float batteryPercentage);

private:
    float calculateBatteryPercentage(float voltage);
};

#endif