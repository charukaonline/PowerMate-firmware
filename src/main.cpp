#include <WiFi.h>

// Define the network credentials
const char *ssid = "ESP32_Hotspot";
const char *password = "password123";

void setup() {
  // Start Serial Monitor for debugging
  Serial.begin(115200);
  Serial.println("Starting Access Point...");

  // Start the Wi-Fi as an access point
  WiFi.softAP(ssid, password);

  // Print the IP address
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP Address: ");
  Serial.println(IP);
}

void loop() {
  // Blink the onboard LED to indicate the ESP32 is running
  const int ledPin = 2;
  pinMode(ledPin, OUTPUT);
  
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
}
