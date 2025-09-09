#include "WiFi.h"

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Wait for Serial to initialize
  delay(1000);
  
  Serial.println("tinyCore ESP32-S3 MAC Address Display");
  Serial.println("========================");
  
  // Initialize WiFi (required to access MAC address)
  WiFi.mode(WIFI_MODE_STA);
  
  // Wait for WiFi to initialize
  delay(10000);

  // Get MAC address as string
  String macAddress = WiFi.macAddress();
  
  // Print MAC address
  Serial.print("MAC Address: ");
  Serial.println(macAddress);
  
  // Print additional network info
  Serial.println("\nAdditional Info:");
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Chip Revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("Flash Size: ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");
}

void loop() {
  // Print MAC address every 10 seconds
  delay(10000);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}