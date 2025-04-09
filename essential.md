// ESP32 receiver code
#include <Arduino.h>

// Define the hardware serial port to use
#define ARDUINO_SERIAL Serial2  // ESP32's UART2 (GPIO16=RX, GPIO17=TX by default)

// Variables to store received values
float dcVoltage = 0.0;
float dcCurrent = 0.0;
float batteryVoltage = 0.0;
float batteryCurrent = 0.0;

void setup() {
  Serial.begin(115200);      // Serial monitor for debugging
  ARDUINO_SERIAL.begin(9600, SERIAL_8N1, 16, 17);  // RX=GPIO16, TX=GPIO17
  
  Serial.println("ESP32 ready to receive data from Arduino");
}

void loop() {
  // Check if data is available from Arduino
  if (ARDUINO_SERIAL.available()) {
    String receivedData = ARDUINO_SERIAL.readStringUntil('\n');
    
    // Parse the comma-separated values
    int index1 = receivedData.indexOf(',');
    int index2 = receivedData.indexOf(',', index1+1);
    int index3 = receivedData.indexOf(',', index2+1);
    
    if (index1 != -1 && index2 != -1 && index3 != -1) {
      dcVoltage = receivedData.substring(0, index1).toFloat();
      dcCurrent = receivedData.substring(index1+1, index2).toFloat();
      batteryVoltage = receivedData.substring(index2+1, index3).toFloat();
      batteryCurrent = receivedData.substring(index3+1).toFloat();
      
      // Print received values for verification
      Serial.println("Received from Arduino:");
      Serial.print("DC: ");
      Serial.print(dcVoltage);
      Serial.print("V, ");
      Serial.print(dcCurrent);
      Serial.print("A | Battery: ");
      Serial.print(batteryVoltage);
      Serial.print("V, ");
      Serial.print(batteryCurrent);
      Serial.println("A");
    }
  }
  
  // Your ESP32 code that uses these values
  // e.g., send to cloud, display on LCD, etc.
  
  delay(100);
}