// For your ESP32 main.cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "./sensors/ultrasonic_sensor.h"
#include "./sensors/temp_sensor.h"

// WiFi credentials
const char* ssid = "CNK2";
const char* password = "fibre2403";

// Server URLs
const char* serverBaseUrl = "http://192.168.1.29:5000/api";
const char* serverUrlAuth = "http://192.168.1.29:5000/api/auth/device";
const char* serverUrlDistance = "http://192.168.1.29:5000/api/distance";
const char* serverUrlDHT11 = "http://192.168.1.29:5000/api/temperature";

// Device credentials
const char* deviceId = "PowerMate-ESP32-001";
const char* deviceSecret = "deviceSecret1";

// JWT token received from server
String authToken = "";

// Create instances for the sensors
HC_SR04 hc_sr04(4, 2);  // HC-SR04 (Trig Pin, Echo Pin)
DHT11_Sensor dht11(17); // DHT11 (Pin 17)

// Function to authenticate device and get token
bool authenticateDevice() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi");
    return false;
  }
  
  HTTPClient http;
  http.begin(serverUrlAuth);
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload with device credentials
  String jsonPayload = "{\"deviceId\":\"" + String(deviceId) + "\",\"deviceSecret\":\"" + String(deviceSecret) + "\"}";
  
  // Send authentication request
  int httpResponseCode = http.POST(jsonPayload);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    // Extract token
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
  
  // If unauthorized, try to re-authenticate
  if (httpResponseCode == 401 || httpResponseCode == 403) {
    Serial.println("Token expired, re-authenticating...");
    if (authenticateDevice()) {
      // Retry the request with new token
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

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Authenticate device at startup
  if (!authenticateDevice()) {
    Serial.println("Initial authentication failed!");
  }

  // Initialize DHT11 sensor
  dht11.begin();
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

  delay(5000);  // Delay before the next loop
}