#include "power_sensor.h"

PowerSensor::PowerSensor(int rx, int tx) : rxPin(rx), txPin(tx) {}

void PowerSensor::begin() {
    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);
}

// Function to calculate battery percentage from voltage
float PowerSensor::calculateBatteryPercentage(float voltage) {
    float maxVoltage = 12.0; // 100% capacity
    float minVoltage = 9.0;  // 0% capacity

    if (voltage >= maxVoltage) return 100.0;
    if (voltage <= minVoltage) return 0.0;

    return ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;
}

// Reads voltage and current values for both DC and Battery from Arduino Uno
bool PowerSensor::readValues(float &dcVoltage, float &dcCurrent, float &batteryVoltage, float &batteryCurrent, float &batteryPercentage) {
    if (Serial2.available()) {
        String receivedData = Serial2.readStringUntil('\n');
        
        // Parse the new format: "DC: [dcV] V, [dcA] A | Battery: [batV] V, [batA] A"
        
        // Find "DC: " position
        int dcPos = receivedData.indexOf("DC: ");
        // Find " | Battery: " position
        int batPos = receivedData.indexOf(" | Battery: ");
        
        if (dcPos != -1 && batPos != -1) {
            // Extract DC part
            String dcPart = receivedData.substring(dcPos + 4, batPos);
            int dcCommaPos = dcPart.indexOf(" V, ");
            
            if (dcCommaPos != -1) {
                // Parse DC voltage
                String dcVoltStr = dcPart.substring(0, dcCommaPos);
                dcVoltage = dcVoltStr.toFloat();
                
                // Parse DC current
                String dcCurrStr = dcPart.substring(dcCommaPos + 4);
                dcCurrStr.replace(" A", "");
                dcCurrent = dcCurrStr.toFloat();
            }
            
            // Extract Battery part
            String batPart = receivedData.substring(batPos + 12);
            int batCommaPos = batPart.indexOf(" V, ");
            
            if (batCommaPos != -1) {
                // Parse Battery voltage
                String batVoltStr = batPart.substring(0, batCommaPos);
                batteryVoltage = batVoltStr.toFloat();
                
                // Parse Battery current
                String batCurrStr = batPart.substring(batCommaPos + 4);
                batCurrStr.replace(" A", "");
                batteryCurrent = batCurrStr.toFloat();
                
                // Calculate battery percentage
                batteryPercentage = calculateBatteryPercentage(batteryVoltage);
            }
            
            return true;
        }
    }
    return false;
}

// Convert values to JSON format
String PowerSensor::getJSON(float dcVoltage, float dcCurrent, float batteryVoltage, float batteryCurrent, float batteryPercentage) {
    String json = "{";
    json += "\"dc\": {";
    json += "\"voltage\": " + String(dcVoltage, 2) + ",";
    json += "\"current\": " + String(dcCurrent, 2);
    json += "},";
    json += "\"battery\": {";
    json += "\"voltage\": " + String(batteryVoltage, 2) + ",";
    json += "\"current\": " + String(batteryCurrent, 2) + ",";
    json += "\"percentage\": " + String(batteryPercentage, 2);
    json += "}";
    json += "}";
    return json;
  }