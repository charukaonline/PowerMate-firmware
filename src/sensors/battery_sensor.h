#ifndef BATTERY_SENSOR_H
#define BATTERY_SENSOR_H

#include <Arduino.h>

class BatterySensor {
private:
    int rxPin, txPin;
    
public:
    BatterySensor(int rx, int tx);
    void begin();
    bool readValues(float &voltage, float &current, float &batteryPercentage);
    String getJSON(float voltage, float current, float batteryPercentage);

private:
    float calculateBatteryPercentage(float voltage);
};

#endif
