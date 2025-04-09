#include "ds18b20_sensor.h"

DS18B20_Sensor::DS18B20_Sensor(int sensorPin) : oneWire(sensorPin), sensors(&oneWire) {
  pin = sensorPin;
}

void DS18B20_Sensor::begin() {
  sensors.begin();
}

float DS18B20_Sensor::getTemperatureC() {
  sensors.requestTemperatures(); // Send the command to get temperatures
  return sensors.getTempCByIndex(0); // Get temperature from first sensor
}

float DS18B20_Sensor::getTemperatureF() {
  sensors.requestTemperatures(); // Send the command to get temperatures
  return sensors.getTempFByIndex(0); // Get temperature in Fahrenheit
}