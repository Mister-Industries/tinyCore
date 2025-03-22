#include <tinyCore_LSM6DS3TRC.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

tinyCore_LSM6DS3TRC lsm6ds3trc;
File dataFile;
const char* DATA_FILENAME = "/imu_data.txt";
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL = 25; // Log every 25ms

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  // Initialize IMU-related pins
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  
  // Initialize I2C
  Serial.println("Starting I2C initialization...");
  Wire.begin(3, 4);
  delay(100);

  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  Serial.println("Scanning for I2C devices...");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      
      // If this is our LSM6DS3TR-C address, try reading WHO_AM_I register
      if (address == 0x6A) {
        Wire.beginTransmission(0x6A);
        Wire.write(0x0F);  // WHO_AM_I register address
        Wire.endTransmission(false);
        Wire.requestFrom(0x6A, 1);
        if (Wire.available()) {
          byte whoAmI = Wire.read();
          Serial.print("WHO_AM_I register value: 0x");
          Serial.println(whoAmI, HEX);
          // Should be 0x6A for LSM6DS3TR-C
        }
      }
    }
  }

  Serial.println("Attempting to initialize LSM6DS3TR-C...");
  if (!lsm6ds3trc.begin_I2C()) {
    Serial.println("Failed to find LSM6DS3TR-C chip");
    Serial.println("Check your wiring!");
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

  // Create/Open data file and write header
  dataFile = SD.open(DATA_FILENAME, FILE_WRITE);
  if (!dataFile) {
    Serial.println("Failed to open data file");
    return;
  }
  
  // Write CSV header
  dataFile.println("timestamp,temperature,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z");
  dataFile.flush();
  
  Serial.println("Logging started!");
  Serial.println("Format: timestamp,temperature,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z");
}

void logIMUData() {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  lsm6ds3trc.getEvent(&accel, &gyro, &temp);
  
  // Format data string
  char dataString[100];
  snprintf(dataString, sizeof(dataString), 
           "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
           millis(),
           temp.temperature,
           accel.acceleration.x,
           accel.acceleration.y,
           accel.acceleration.z,
           gyro.gyro.x,
           gyro.gyro.y,
           gyro.gyro.z);
  
  // Write to SD card
  dataFile.println(dataString);
  dataFile.flush();  // Ensure data is written to card
  
  // Also print to Serial for monitoring
  Serial.println(dataString);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Log data at specified interval
  if (currentTime - lastLogTime >= LOG_INTERVAL) {
    logIMUData();
    lastLogTime = currentTime;
  }
}