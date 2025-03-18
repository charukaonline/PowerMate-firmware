#include "battery_sensor.h"

BatterySensor::BatterySensor(int rx, int tx) : rxPin(rx), txPin(tx) {}

void BatterySensor::begin() {
    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);
}

// Function to calculate battery percentage from voltage
float BatterySensor::calculateBatteryPercentage(float voltage) {
    float maxVoltage = 12.0; // 100% capacity
    float minVoltage = 9.0;  // 0% capacity

    if (voltage >= maxVoltage) return 100.0;
    if (voltage <= minVoltage) return 0.0;

    return ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;
}

// Reads voltage and current values from Arduino Uno
bool BatterySensor::readValues(float &voltage, float &current, float &batteryPercentage) {
    if (Serial2.available()) {
        String receivedData = Serial2.readStringUntil('\n');

        int commaIndex = receivedData.indexOf(" V, ");
        if (commaIndex != -1) {
            String voltageStr = receivedData.substring(0, commaIndex);
            voltage = voltageStr.toFloat();

            String currentStr = receivedData.substring(commaIndex + 4);
            currentStr.trim();
            currentStr.replace(" A", "");
            current = currentStr.toFloat();

            batteryPercentage = calculateBatteryPercentage(voltage);

            return true;
        }
    }
    return false;
}

// Convert values to JSON format
String BatterySensor::getJSON(float voltage, float current, float batteryPercentage) {
    String jsonPayload = "{\"voltage\":" + String(voltage, 2) +
                         ",\"current\":" + String(current, 2) +
                         ",\"batteryPercentage\":" + String(batteryPercentage, 2) + "}";
    return jsonPayload;
}
