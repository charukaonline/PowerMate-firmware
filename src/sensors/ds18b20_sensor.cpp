#include "ds18b20_sensor.h"

DS18B20_Sensor::DS18B20_Sensor(int dataPin) : pin(dataPin) {
    oneWire = new OneWire(pin);
    sensors = new DallasTemperature(oneWire);
}

void DS18B20_Sensor::begin() {
    sensors->begin();
}

float DS18B20_Sensor::getTemperatureC() {
    sensors->requestTemperatures(); 
    return sensors->getTempCByIndex(0); // Get temperature in Celsius
}

float DS18B20_Sensor::getTemperatureF() {
    sensors->requestTemperatures(); 
    return sensors->getTempFByIndex(0); // Get temperature in Fahrenheit
}
