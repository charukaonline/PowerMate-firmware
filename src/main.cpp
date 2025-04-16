#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char *ssid = "CNK2";
const char *password = "fibre2403";

// Server URL - simplified
const char *serverUrlSensorData = "http://192.168.1.37:5000/api/sensor-data/temp-data";

// Data wire is connected to GPIO4 (can be changed)
#define ONE_WIRE_BUS 14

// Setup oneWire and DallasTemperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

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
}

void sendTemperatureData(float temperature)
{
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        // Specify the URL
        http.begin(serverUrlSensorData);

        // Specify content-type header
        http.addHeader("Content-Type", "application/json");

        // Create JSON document
        DynamicJsonDocument doc(200);
        doc["temperature"] = temperature;

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
