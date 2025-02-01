#include "temp_sensor.h"

DHT11_Sensor::DHT11_Sensor(int sensorPin) : dht(sensorPin, DHT11) {
  pin = sensorPin;
}

void DHT11_Sensor::begin() {
  dht.begin();
}

float DHT11_Sensor::getTemperatureC() {
  return dht.readTemperature(false);  // Celsius
}

float DHT11_Sensor::getTemperatureF() {
  return dht.readTemperature(true);   // Fahrenheit
}

float DHT11_Sensor::getHumidity() {
  return dht.readHumidity();          // Humidity
}
