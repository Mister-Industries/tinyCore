#include "WiFi.h"

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);

  Serial.println("tinyCore ESP32-S3 MAC Address Display");
  Serial.println("=====================================");

  // Initialize WiFi (required to access MAC address)
  WiFi.mode(WIFI_MODE_STA);
  delay(100);

  // Get and print MAC address
  String macAddress = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macAddress);

  // Print additional device info
  Serial.println("\nDevice Info:");
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
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