// For your ESP32 main.cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "./sensors/ultrasonic_sensor.h"
#include "./sensors/temp_sensor.h"
#include "./sensors/battery_sensor.h"  // Renamed from PowerSensor

// WiFi credentials
const char* ssid = "CNK2";
const char* password = "fibre2403";

// Server URLs
const char* serverBaseUrl = "http://192.168.1.29:5000/api";
const char* serverUrlAuth = "http://192.168.1.29:5000/api/auth/device";
const char* serverUrlDistance = "http://192.168.1.29:5000/api/distance";
const char* serverUrlDHT11 = "http://192.168.1.29:5000/api/temperature";
const char* serverUrlBattery = "http://192.168.1.29:5000/api/battery"; // New API for battery readings

// Device credentials
const char* deviceId = "PowerMate-ESP32-001";
const char* deviceSecret = "deviceSecret1";

// JWT token received from server
String authToken = "";

// Create instances for the sensors
HC_SR04 hc_sr04(4, 2);   // HC-SR04 (Trig Pin, Echo Pin)
DHT11_Sensor dht11(18);  // DHT11 (Pin 17)
BatterySensor batterySensor(16, 17); // BatterySensor (RX, TX)

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

    if (httpResponseCode == 200) {
        String response = http.getString();

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        authToken = doc["token"].as<String>();
        Serial.println("Device authenticated successfully");
        return true;
    } else {
        Serial.print("Authentication failed, HTTP response code: ");
        Serial.println(httpResponseCode);
        return false;
    }

    http.end();
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

    if (!authenticateDevice()) {
        Serial.println("Initial authentication failed!");
    }

    dht11.begin();
    batterySensor.begin();
}

void loop() {
    // HC-SR04 Distance measurement
    float distance = hc_sr04.getDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // DHT11 Temperature and Humidity readings
    float tempC = dht11.getTemperatureC();
    float tempF = dht11.getTemperatureF();
    float humidity = dht11.getHumidity();

    Serial.print("Temp: ");
    Serial.print(tempC);
    Serial.print(" C, ");
    Serial.print(tempF);
    Serial.print(" F, Hum: ");
    Serial.print(humidity);
    Serial.println("%");

    // Sending distance data to the server
    String jsonPayloadDistance = "{\"distance\":" + String(distance, 2) + "}";
    if (!sendDataToAPI(serverUrlDistance, jsonPayloadDistance)) {
        Serial.println("Error sending data to /api/distance");
    }

    // Sending DHT11 sensor data to the server
    String jsonPayloadDHT11 = "{\"temperatureC\":" + String(tempC, 2) + 
                           ",\"temperatureF\":" + String(tempF, 2) + 
                           ",\"humidity\":" + String(humidity, 2) + "}";
    if (!sendDataToAPI(serverUrlDHT11, jsonPayloadDHT11)) {
        Serial.println("Error sending data to /api/temperature");
    }

    // BatterySensor: Voltage, Current, Battery Percentage
    float voltage, current, batteryPercentage;
    if (batterySensor.readValues(voltage, current, batteryPercentage)) {
        Serial.print("Voltage: ");
        Serial.print(voltage);
        Serial.print(" V | Current: ");
        Serial.print(current);
        Serial.print(" A | Battery: ");
        Serial.print(batteryPercentage);
        Serial.println("%");

        // Send battery readings to API
        String jsonPayloadBattery = batterySensor.getJSON(voltage, current, batteryPercentage);
        if (!sendDataToAPI(serverUrlBattery, jsonPayloadBattery)) {
            Serial.println("Error sending battery data to /api/battery");
        }
    }

    delay(5000);
}
