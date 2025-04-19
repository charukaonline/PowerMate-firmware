#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "./sensors/ultrasonic_sensor.h"
#include "./sensors/power_sensor.h"

// WiFi credentials
const char *ssid = "CNK2";
const char *password = "fibre2403";

// Server URLs
const char *serverUrlSensorData = "http://192.168.1.37:5000/api/sensor-data/data";
const char *serverUrlAuth = "http://192.168.1.37:5000/api/auth/device";

// Create instances for the sensors
HC_SR04 hc_sr04(4, 2);           // HC-SR04 (Trig Pin, Echo Pin)
PowerSensor powerSensor(16, 17); // PowerSensor (RX, TX)

// Authentication variables
String deviceId;
String authToken;
bool isAuthenticated = false;

// Get device ID from MAC address
String getDeviceId()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18] = {0};
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

// Authenticate device with server
bool authenticateDevice()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Not connected to WiFi. Cannot authenticate.");
        return false;
    }

    HTTPClient http;
    http.begin(serverUrlAuth);
    http.addHeader("Content-Type", "application/json");

    // Create authentication request
    DynamicJsonDocument doc(200);
    doc["deviceId"] = deviceId;
    doc["deviceType"] = "esp32-01";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);

    if (httpCode == 200 || httpCode == 201)
    {
        String response = http.getString();
        DynamicJsonDocument responseDoc(1024);
        DeserializationError error = deserializeJson(responseDoc, response);

        if (!error && responseDoc.containsKey("token"))
        {
            authToken = responseDoc["token"].as<String>();
            Serial.println("Device authenticated successfully");
            http.end();
            return true;
        }
    }

    Serial.print("Authentication failed with code: ");
    Serial.println(httpCode);
    if (httpCode > 0)
    {
        String response = http.getString();
        Serial.println("Response: " + response);
    }
    http.end();
    return false;
}

// function to send sensor data with device authentication
bool sendAllSensorData(float distance,
                       float dcVoltage, float dcCurrent,
                       float batteryVoltage, float batteryCurrent, float batteryPercentage)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Not connected to WiFi");
        return false;
    }

    // Check authentication first
    if (!isAuthenticated)
    {
        Serial.println("Not authenticated. Attempting to authenticate...");
        isAuthenticated = authenticateDevice();
        if (!isAuthenticated)
        {
            Serial.println("Authentication failed. Cannot send data.");
            return false;
        }
    }

    // Create a JSON document for all sensor data
    DynamicJsonDocument doc(1024);

    // Add device ID
    doc["deviceId"] = deviceId;

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
    http.addHeader("Authorization", "Bearer " + authToken);

    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Get and print response
    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println("Response: " + response);

        // Handle authentication errors
        if (httpResponseCode == 401 || httpResponseCode == 403)
        {
            Serial.println("Auth token expired or invalid. Reauthenticating...");
            isAuthenticated = false;
            // try again in the next loop
        }
    }
    else
    {
        // detailed error based on error code
        Serial.print("Error sending HTTP request. Error code: ");
        Serial.print("Error description: ");
        if (httpResponseCode == -1)
        {
            Serial.println("Connection failed or timed out. Check server availability and URL.");
        }
        else
        {
            Serial.println(http.errorToString(httpResponseCode));
        }
    }

    http.end();
    return (httpResponseCode == 200 || httpResponseCode == 201);
}

void setup()
{
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);

    // add 10 second timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
    {
        delay(1000);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nFailed to connect to WiFi. Check credentials or signal strength.");
    }
    else
    {
        Serial.println("\nConnected to WiFi");
        // Print connection info
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Get device ID and authenticate
        deviceId = getDeviceId();
        Serial.print("Device ID: ");
        Serial.println(deviceId);

        isAuthenticated = authenticateDevice();
        if (!isAuthenticated)
        {
            Serial.println("Initial authentication failed. Will retry during sensor readings.");
        }
    }

    // Initialize sensors
    powerSensor.begin();
}

void loop()
{
    // Collect all sensor data
    // HC-SR04 Distance measurement
    float distance = hc_sr04.getDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Power Sensor measurements
    float dcVoltage = 0, dcCurrent = 0, batteryVoltage = 0, batteryCurrent = 0, batteryPercentage = 0;
    bool powerReadSuccess = powerSensor.readValues(
        dcVoltage, dcCurrent, batteryVoltage, batteryCurrent, batteryPercentage);

    if (powerReadSuccess)
    {
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
    }
    else
    {
        Serial.println("Failed to read power sensor data");
    }

    // Only send data if we have valid readings
    if (powerReadSuccess)
    {
        // Send all data in one API call
        if (!sendAllSensorData(
                distance,
                dcVoltage, dcCurrent,
                batteryVoltage, batteryCurrent, batteryPercentage))
        {
            Serial.println("Error sending data to unified API endpoint");
        }
        else
        {
            Serial.println("Successfully sent all sensor data to server");
        }
    }
    else
    {
        Serial.println("Skipping API call due to invalid sensor readings");
    }

    delay(5000); // Wait 5 seconds before next reading
}