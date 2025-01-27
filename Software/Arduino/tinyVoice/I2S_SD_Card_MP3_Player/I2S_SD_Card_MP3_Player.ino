#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <driver/i2s.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// Pin definitions
#define I2S_BCLK_PIN       8  // Bit clock
#define I2S_LRCLK_PIN      9  // Left/Right clock (Word Select)
#define I2S_DATA_PIN       10  // Data pin

// Audio objects
AudioFileSourceSD *file;
AudioFileSourceID3 *id3;
AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    Serial.println("Initializing...");

    // Initialize SD card using the working method
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

    // Check if test.mp3 exists
    if (!SD.exists("/test.mp3")) {
        Serial.println("Can't find /test.mp3");
        return;
    }
    Serial.println("Found test.mp3");

    // Set up I2S output
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DATA_PIN);
    out->SetGain(0.5); // Set volume (0.0-1.0)

    // Set up MP3 decoder
    file = new AudioFileSourceSD("/test.mp3");
    id3 = new AudioFileSourceID3(file);
    mp3 = new AudioGeneratorMP3();

    Serial.println("Starting MP3...");
    mp3->begin(id3, out);
}

void loop() {
    if (mp3->isRunning()) {
        if (!mp3->loop()) {
            // File is finished playing
            mp3->stop();
            Serial.println("MP3 playback completed");
            delay(1000);

            // Restart playback
            Serial.println("Restarting playback...");
            file->open("/test.mp3");
            mp3->begin(id3, out);
        }
    } else {
        Serial.println("MP3 not running, attempting to restart...");
        delay(1000);
        file->open("/test.mp3");
        mp3->begin(id3, out);
    }
}