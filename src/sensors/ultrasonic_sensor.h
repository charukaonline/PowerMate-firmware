#ifndef HC_SR04_H
#define HC_SR04_H

#include <Arduino.h>

class HC_SR04 {
  private:
    int trigPin;
    int echoPin;
  
  public:
    HC_SR04(int trig, int echo);  // Constructor to initialize pins
    float getDistance();          // Method to get the distance in cm
};

#endif
