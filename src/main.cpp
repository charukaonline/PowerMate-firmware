#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char *ssid = "CNK2";
const char *password = "fibre2403";

// Server URLs
const char *serverUrlSensorData = "http://192.168.1.37:5000/api/sensor-data/temp-data";
const char *serverUrlAuth = "http://192.168.1.37:5000/api/auth/device";

// Authentication
String deviceId;
String authToken;
bool isAuthenticated = false;

// Data wire is connected to GPIO4 (can be changed)
#define ONE_WIRE_BUS 14

// Setup oneWire and DallasTemperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

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
    HTTPClient http;
    http.begin(serverUrlAuth);
    http.addHeader("Content-Type", "application/json");

    // Create authentication request
    DynamicJsonDocument doc(200);
    doc["deviceId"] = deviceId;
    // You might want to add a device secret or key here
    doc["deviceType"] = "esp32-02";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);

    if (httpCode == 200)
    {
        String response = http.getString();
        DynamicJsonDocument responseDoc(1024);
        deserializeJson(responseDoc, response);

        // Store authentication token
        if (responseDoc.containsKey("token"))
        {
            authToken = responseDoc["token"].as<String>();
            Serial.println("Device authenticated successfully");
            http.end();
            return true;
        }
    }

    Serial.print("Authentication failed with code: ");
    Serial.println(httpCode);
    http.end();
    return false;
}

void setup()
{
    Serial.begin(115200);
    sensors.begin();

    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
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

void sendTemperatureData(float temperature)
{
    // First check if we're authenticated
    if (!isAuthenticated)
    {
        Serial.println("Not authenticated. Attempting to authenticate...");
        isAuthenticated = authenticateDevice();
        if (!isAuthenticated)
        {
            Serial.println("Authentication failed. Cannot send data.");
            return;
        }
    }

    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        // Specify the URL
        http.begin(serverUrlSensorData);

        // Specify content-type and auth headers
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + authToken);

        // Create JSON document
        DynamicJsonDocument doc(200);
        doc["temperature"] = temperature;
        doc["deviceId"] = deviceId;

        // Serialize JSON document
        String json;
        serializeJson(doc, json);

        // Send HTTP POST request
        int httpResponseCode = http.POST(json);

        // Check response
        if (httpResponseCode > 0)
        {
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Response: " + response);
        }
        else if (httpResponseCode == 401 || httpResponseCode == 403)
        {
            Serial.println("Auth token expired or invalid. Reauthenticating...");
            isAuthenticated = false;
            // Will try again in the next loop
        }
        else
        {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }

        // Free resources
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
}

void loop()
{
    sensors.requestTemperatures();            // Send the command to get temperatures
    float tempC = sensors.getTempCByIndex(0); // Get temperature of first sensor

    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println(" °C");

    // Send temperature data to API
    sendTemperatureData(tempC);

    delay(5000); // Send data every 5 seconds
}
