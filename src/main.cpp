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

// Server URLs
const char* serverBaseUrl = "http://192.168.1.37:5000/api";
const char* serverUrlAuth = "http://192.168.1.37:5000/api/auth/device";
const char* serverUrlDistance = "http://192.168.1.37:5000/api/distance";
const char* serverUrlTemp = "http://192.168.1.37:5000/api/temperature";
const char* serverUrlPower = "http://192.168.1.37:5000/api/power";

// Device credentials
const char* deviceId = "tower1";
const char* deviceSecret = "cnk12345";

// JWT token received from server
String authToken = "";

// Create instances for the sensors
HC_SR04 hc_sr04(4, 2);   // HC-SR04 (Trig Pin, Echo Pin)
DS18B20_Sensor ds18b20(18);  // DS18B20 on pin 18 (same pin as previous DHT11)
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
        success = true;
    } else {
        Serial.print("Authentication failed, HTTP response code: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return success;
}

// Add this function to test basic server connectivity
bool testServerConnectivity() {
    HTTPClient http;
    http.begin(serverBaseUrl);
    int httpCode = http.GET();
    Serial.print("Server connectivity test: ");
    Serial.println(httpCode);
    http.end();
    return httpCode > 0;
}

// Function to send data to API with authentication
bool sendDataToAPI(const char* url, String jsonPayload) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + authToken);

    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 401 || httpResponseCode == 403) {
        Serial.println("Token expired, re-authenticating...");
        if (authenticateDevice()) {
            http.begin(url);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Authorization", "Bearer " + authToken);
            httpResponseCode = http.POST(jsonPayload);

            Serial.print("Retry HTTP Response code: ");
            Serial.println(httpResponseCode);
        }
    }

    http.end();
    return (httpResponseCode == 200 || httpResponseCode == 201);
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Add this in setup() after WiFi connection
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Attempting to connect to server at: ");
    Serial.println(serverBaseUrl);
    if (!testServerConnectivity()) {
        Serial.println("Warning: Cannot reach server!");
    }

    if (!authenticateDevice()) {
        Serial.println("Initial authentication failed!");
    }

    ds18b20.begin(); // Changed from dht11.begin()
    powerSensor.begin();
}

void loop() {
    // HC-SR04 Distance measurement
    float distance = hc_sr04.getDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // DS18B20 Temperature readings (no humidity)
    float tempC = ds18b20.getTemperatureC();
    float tempF = ds18b20.getTemperatureF();

    Serial.print("Temp: ");
    Serial.print(tempC);
    Serial.print(" C, ");
    Serial.print(tempF);
    Serial.println(" F");

    // Sending distance data to the server
    String jsonPayloadDistance = "{\"distance\":" + String(distance, 2) + "}";
    if (!sendDataToAPI(serverUrlDistance, jsonPayloadDistance)) {
        Serial.println("Error sending data to /api/distance");
    }

    // Sending Temperature sensor data to the server (no humidity)
    String jsonPayloadTemp = "{\"temperatureC\":" + String(tempC, 2) + 
                          ",\"temperatureF\":" + String(tempF, 2) + "}";
    if (!sendDataToAPI(serverUrlTemp, jsonPayloadTemp)) {
        Serial.println("Error sending data to /api/temperature");
    }

    // Power Sensor measurements (no changes)
    float dcVoltage, dcCurrent, batteryVoltage, batteryCurrent, batteryPercentage;
    if (powerSensor.readValues(dcVoltage, dcCurrent, batteryVoltage, batteryCurrent, batteryPercentage)) {
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

        // Send power readings to API
        String jsonPayloadPower = powerSensor.getJSON(dcVoltage, dcCurrent, batteryVoltage, batteryCurrent, batteryPercentage);
        if (!sendDataToAPI(serverUrlPower, jsonPayloadPower)) {
            Serial.println("Error sending power data to /api/power");
        }
    }

    delay(5000);
}
