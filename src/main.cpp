// For your ESP32 main.cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "./sensors/ultrasonic_sensor.h"
#include "./sensors/ds18b20_sensor.h" // Changed from temp_sensor.h
#include "./sensors/power_sensor.h" 

// WiFi credentials
const char* ssid = "CNK2";
const char* password = "fibre2403";

// Server URLs - updated to use the unified endpoint
const char* serverBaseUrl = "http://192.168.1.37:5000/api";
const char* serverUrlAuth = "http://192.168.1.37:5000/api/auth";
const char* serverUrlSensorData = "http://192.168.1.37:5000/api/sensor-data";

// Device credentials
const char* deviceId = "tower1";
const char* deviceSecret = "cnk12345";

// JWT token received from server
String authToken = "";

// Create instances for the sensors
HC_SR04 hc_sr04(4, 2);   // HC-SR04 (Trig Pin, Echo Pin)
DS18B20_Sensor ds18b20(18);  // DS18B20 on pin 18
PowerSensor powerSensor(16, 17); // PowerSensor (RX, TX)

// Function to authenticate device and get token
bool authenticateDevice() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Not connected to WiFi");
        return false;
    }

    HTTPClient http;
    http.begin(serverUrlAuth);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"deviceId\":\"" + String(deviceId) + "\",\"deviceSecret\":\"" + String(deviceSecret) + "\"}";

    int httpResponseCode = http.POST(jsonPayload);
    bool success = false;

    if (httpResponseCode == 200) {
        String response = http.getString();

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        authToken = doc["token"].as<String>();
        Serial.println("Device authenticated successfully");
        Serial.println("Token: " + authToken);
        success = true;
    } else {
        Serial.print("Authentication failed, HTTP response code: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return success;
}

// Add this function to test basic server connectivity
// bool testServerConnectivity() {
//     HTTPClient http;
//     http.begin(serverBaseUrl + String("/ping")); // Use the ping endpoint for connectivity test
//     int httpCode = http.GET();
//     Serial.print("Server connectivity test: ");
//     Serial.println(httpCode);
//     http.end();
//     return httpCode > 0;
// }

// New function to send all sensor data in a single API call
bool sendAllSensorData(float distance, float tempC, float tempF, 
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

    // Add temperature data
    JsonObject temperatureObj = doc.createNestedObject("temperature");
    temperatureObj["temperatureC"] = tempC;
    temperatureObj["temperatureF"] = tempF;

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
    
    Serial.println("Sending unified sensor data:");
    Serial.println(jsonPayload);

    // Send HTTP request
    HTTPClient http;
    http.begin(serverUrlSensorData);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + authToken);

    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Handle authentication errors
    if (httpResponseCode == 401 || httpResponseCode == 403) {
        Serial.println("Token expired, re-authenticating...");
        if (authenticateDevice()) {
            http.begin(serverUrlSensorData);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Authorization", "Bearer " + authToken);
            httpResponseCode = http.POST(jsonPayload);

            Serial.print("Retry HTTP Response code: ");
            Serial.println(httpResponseCode);
        }
    }

    // Get and print response
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Response: " + response);
    }

    http.end();
    return (httpResponseCode == 200 || httpResponseCode == 201);
}

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Print connection info
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Attempting to connect to server at: ");
    Serial.println(serverBaseUrl);
    
    // // Test connectivity
    // if (!testServerConnectivity()) {
    //     Serial.println("Warning: Cannot reach server!");
    // }

    // Get initial authentication token
    if (!authenticateDevice()) {
        Serial.println("Initial authentication failed!");
    }

    // Initialize sensors
    ds18b20.begin();
    powerSensor.begin();
}

void loop() {
    // Collect all sensor data
    
    // HC-SR04 Distance measurement
    float distance = hc_sr04.getDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // DS18B20 Temperature readings
    float tempC = ds18b20.getTemperatureC();
    float tempF = ds18b20.getTemperatureF();

    Serial.print("Temp: ");
    Serial.print(tempC);
    Serial.print(" C, ");
    Serial.print(tempF);
    Serial.println(" F");

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

    // Send all data in one API call
    if (!sendAllSensorData(
            distance, tempC, tempF, 
            dcVoltage, dcCurrent, 
            batteryVoltage, batteryCurrent, batteryPercentage)) {
        Serial.println("Error sending data to unified API endpoint");
    } else {
        Serial.println("Successfully sent all sensor data to server");
    }

    delay(5000); // Wait 5 seconds before next reading
}