// For your ESP32 main.cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "./sensors/ultrasonic_sensor.h"
#include "./sensors/power_sensor.h" 

// WiFi credentials
const char* ssid = "CNK2";
const char* password = "fibre2403";

// Server URL - simplified
const char* serverUrlSensorData = "http://192.168.1.37:5000/api/sensor-data/data";

// Create instances for the sensors
HC_SR04 hc_sr04(4, 2);   // HC-SR04 (Trig Pin, Echo Pin)
PowerSensor powerSensor(16, 17); // PowerSensor (RX, TX)

// Simplified function to send sensor data without device ID
bool sendAllSensorData(float distance,
                      float dcVoltage, float dcCurrent,
                      float batteryVoltage, float batteryCurrent, float batteryPercentage) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Not connected to WiFi");
        return false;
    }

    // Create a JSON document for all sensor data
    DynamicJsonDocument doc(1024);

    // Add distance data
    JsonObject distanceObj = doc.createNestedObject("distance");
    distanceObj["distance"] = distance;

    // Add DC power data
    JsonObject dcPowerObj = doc.createNestedObject("dcPower");
    dcPowerObj["voltage"] = dcVoltage;
    dcPowerObj["current"] = dcCurrent;

    // Add battery data
    JsonObject batteryObj = doc.createNestedObject("battery");
    batteryObj["voltage"] = batteryVoltage;
    batteryObj["current"] = batteryCurrent;
    batteryObj["percentage"] = batteryPercentage;

    // Serialize JSON to string
    String jsonPayload;
    serializeJson(doc, jsonPayload);
    
    // Send HTTP request
    HTTPClient http;
    http.begin(serverUrlSensorData);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Get and print response
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Response: " + response);
    } else {
        // Print detailed error based on error code
        Serial.print("Error sending HTTP request. Error code: ");
        Serial.print("Error description: ");
        if (httpResponseCode == -1) {
            Serial.println("Connection failed or timed out. Check server availability and URL.");
        } else {
            Serial.println(http.errorToString(httpResponseCode));
        }
    }
    
    http.end();
    return (httpResponseCode == 200 || httpResponseCode == 201);
}

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    
    // Set timeout for WiFi connection - add 10 second timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(1000);
        Serial.print(".");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi. Check credentials or signal strength.");
    } else {
        Serial.println("\nConnected to WiFi");
        // Print connection info
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    
    // Initialize sensors
    powerSensor.begin();
}

void loop() {
    // Collect all sensor data
    // HC-SR04 Distance measurement
    float distance = hc_sr04.getDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Power Sensor measurements
    float dcVoltage = 0, dcCurrent = 0, batteryVoltage = 0, batteryCurrent = 0, batteryPercentage = 0;
    bool powerReadSuccess = powerSensor.readValues(
        dcVoltage, dcCurrent, batteryVoltage, batteryCurrent, batteryPercentage
    );

    if (powerReadSuccess) {
        Serial.print("DC: ");
        Serial.print(dcVoltage);
        Serial.print(" V, ");
        Serial.print(dcCurrent);
        Serial.print(" A | Battery: ");
        Serial.print(batteryVoltage);
        Serial.print(" V, ");
        Serial.print(batteryCurrent);
        Serial.print(" A, ");
        Serial.print(batteryPercentage);
        Serial.println("%");
    } else {
        Serial.println("Failed to read power sensor data");
    }

    // Only send data if we have valid readings
    if (powerReadSuccess) {  // Basic check to filter out invalid temperature readings
        // Send all data in one API call
        if (!sendAllSensorData(
                distance, 
                dcVoltage, dcCurrent, 
                batteryVoltage, batteryCurrent, batteryPercentage)) {
            Serial.println("Error sending data to unified API endpoint");
        } else {
            Serial.println("Successfully sent all sensor data to server");
        }
    } else {
        Serial.println("Skipping API call due to invalid sensor readings");
    }

    delay(5000); // Wait 5 seconds before next reading
}