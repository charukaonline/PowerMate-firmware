#ifndef DHT11_SENSOR_H
#define DHT11_SENSOR_H

#include <DHT.h>

class DHT11_Sensor {
  private:
    int pin;
    DHT dht;

  public:
    DHT11_Sensor(int sensorPin);
    void begin();
    float getTemperatureC();
    float getTemperatureF();
    float getHumidity();
};

#endif
