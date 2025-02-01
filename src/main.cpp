#include <WiFi.h>
#include <HTTPClient.h>
#include "./sensors/ultrasonic_sensor.h"
#include "./sensors/temp_sensor.h"

// WiFi credentials
const char* ssid = "CNK2";
const char* password = "fibre2403";

// Server URLs
const char* serverUrlDistance = "http://192.168.1.37:5000/api/distance";
const char* serverUrlDHT11 = "http://192.168.1.37:5000/api/temperature";

// Create instances for the sensors
HC_SR04 hc_sr04(4, 2);  // HC-SR04 (Trig Pin, Echo Pin)
DHT11_Sensor dht11(17); // DHT11 (Pin 17)

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

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

  // Sending distance data to the server (API for Distance)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrlDistance); // Distance API
    http.addHeader("Content-Type", "application/json");

    String jsonPayloadDistance = "{\"distance\":" + String(distance, 2) + "}";
    int httpResponseCode = http.POST(jsonPayloadDistance);

    Serial.print("Distance HTTP Response code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode != 200) {
      Serial.println("Error sending data to /api/distance");
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  // Sending DHT11 sensor data to the server (API for DHT11)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrlDHT11); // DHT11 API
    http.addHeader("Content-Type", "application/json");

    String jsonPayloadDHT11 = "{\"temperatureC\":" + String(tempC, 2) + ",\"temperatureF\":" + String(tempF, 2) + ",\"humidity\":" + String(humidity, 2) + "}";
    int httpResponseCode = http.POST(jsonPayloadDHT11);

    Serial.print("DHT11 HTTP Response code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode != 200) {
      Serial.println("Error sending data to /api/temperature");
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(5000);  // Delay before the next loop
}
