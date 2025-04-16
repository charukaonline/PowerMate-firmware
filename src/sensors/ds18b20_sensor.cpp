#include "ds18b20_sensor.h"

DS18B20_Sensor::DS18B20_Sensor(int dataPin) : pin(dataPin) {
    oneWire = new OneWire(pin);
    sensors = new DallasTemperature(oneWire);
}

void DS18B20_Sensor::begin() {
    sensors->begin();
    sensors->setResolution(12); // Set to highest resolution (12-bit)
    sensors->setWaitForConversion(true);
}

float DS18B20_Sensor::getTemperatureC() {
    // Try up to 3 times to get a valid reading
    for (int i = 0; i < 3; i++) {
        sensors->requestTemperatures();
        float temp = sensors->getTempCByIndex(0);
        
        // Check if reading is valid (-127.0 is the error value for DS18B20)
        if (temp > -100.0) {
            lastValidTempC = temp; // Save valid reading
            return temp;
        }
        delay(100); // Wait before retrying
    }
    
    // Return last valid reading if available, otherwise return default 30°C
    if (lastValidTempC != -1000.0) {
        Serial.println("Using last valid temperature reading");
        return lastValidTempC;
    } else {
        Serial.println("Using default temperature: 30°C");
        return 30.0; // Return explicit 30°C instead of error value
    }
}

float DS18B20_Sensor::getTemperatureF() {
    float tempC = getTemperatureC();
    // Only convert if we have a valid reading
    if (tempC > -100.0) {
        return sensors->toFahrenheit(tempC);
    }
    return 86.0; // Default 30°C in Fahrenheit (86°F)
}

bool DS18B20_Sensor::isReadingValid(float tempC) {
    return (tempC > -100.0);
}