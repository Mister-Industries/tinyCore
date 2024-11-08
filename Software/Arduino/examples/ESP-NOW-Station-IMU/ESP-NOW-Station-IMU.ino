#include "ESP32_NOW_Serial.h"
#include "MacAddress.h"
#include "WiFi.h"
#include "esp_wifi.h"
#include <Adafruit_LSM6DS3TRC.h>

// ESP-NOW Configuration
#define ESPNOW_WIFI_MODE_STATION 1
#define ESPNOW_WIFI_CHANNEL 1

#if ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_STA
#define ESPNOW_WIFI_IF   WIFI_IF_STA
#else
#define ESPNOW_WIFI_MODE WIFI_AP
#define ESPNOW_WIFI_IF   WIFI_IF_AP
#endif

// Set your receiver's MAC address here
const MacAddress peer_mac({0xF2, 0xF5, 0xBD, 0x50, 0xB0, 0x10});

ESP_NOW_Serial_Class NowSerial(peer_mac, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);
Adafruit_LSM6DS3TRC lsm6ds3trc;

// Buffer for formatting IMU data
char dataBuffer[100];

void setup() {
  Serial.begin(115200);
  
  // Initialize IMU-related pins
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  
  // Initialize I2C
  Wire.begin(3, 4);
  delay(100);

  // Initialize WiFi for ESP-NOW
  Serial.print("WiFi Mode: ");
  Serial.println(ESPNOW_WIFI_MODE == WIFI_AP ? "AP" : "Station");
  WiFi.mode(ESPNOW_WIFI_MODE);

  Serial.print("Channel: ");
  Serial.println(ESPNOW_WIFI_CHANNEL);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  while (!(WiFi.STA.started() || WiFi.AP.started())) {
    delay(100);
  }

  Serial.print("MAC Address: ");
  Serial.println(ESPNOW_WIFI_MODE == WIFI_AP ? WiFi.softAPmacAddress() : WiFi.macAddress());

  // Initialize ESP-NOW
  Serial.println("ESP-NOW communication starting...");
  NowSerial.begin(115200);

  // Initialize LSM6DS3TR-C
  Serial.println("Initializing LSM6DS3TR-C...");
  if (!lsm6ds3trc.begin_I2C()) {
    Serial.println("Failed to find LSM6DS3TR-C chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("LSM6DS3TR-C Found!");

  // Configure IMU settings
  lsm6ds3trc.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
  lsm6ds3trc.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
  lsm6ds3trc.setAccelDataRate(LSM6DS_RATE_104_HZ);
  lsm6ds3trc.setGyroDataRate(LSM6DS_RATE_104_HZ);

  lsm6ds3trc.configInt1(false, false, true);  // accelerometer DRDY on INT1
  lsm6ds3trc.configInt2(false, true, false);  // gyro DRDY on INT2
}

void loop() {
  // Read IMU data
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  lsm6ds3trc.getEvent(&accel, &gyro, &temp);

  // Format the data into a string
  // Format: "temp,ax,ay,az,gx,gy,gz"
  snprintf(dataBuffer, sizeof(dataBuffer), "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
           temp.temperature,
           accel.acceleration.x,
           accel.acceleration.y,
           accel.acceleration.z,
           gyro.gyro.x,
           gyro.gyro.y,
           gyro.gyro.z);

  // Send the data over ESP-NOW
  if (NowSerial.availableForWrite()) {
    for (size_t i = 0; i < strlen(dataBuffer); i++) {
      NowSerial.write(dataBuffer[i]);
    }
    NowSerial.write('\n');  // Add newline for easier parsing on receiver side
  }

  // Also print to Serial for debugging
  Serial.println(dataBuffer);

  // Read any incoming ESP-NOW data (if needed)
  while (NowSerial.available()) {
    Serial.write(NowSerial.read());
  }

  delay(100);  // Adjust this delay based on your needs
}