#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <driver/i2s.h>

// Pin Definitions
// Microphone (SPH0645)
#define I2S_MIC_SCK     8
#define I2S_MIC_WS      9
#define I2S_MIC_SD      10

// Speaker
#define I2S_SPKR_BCLK   SDA //3
#define I2S_SPKR_LRC    TX //39
#define I2S_SPKR_DIN    SCL //4

// Constants for recording
const int RECORD_TIME = 5;  // seconds to record
const int SAMPLE_RATE = 44100;
const int SAMPLE_BITS = 32;
const int WAV_HEADER_SIZE = 44;
const char* RECORD_FILE = "/recording.wav";
const int BINARY_BUFFER_SIZE = 1024;

// Global flag to track I2S state
bool i2s_initialized = false;

// WAV header structure
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t byteRate = SAMPLE_RATE * 2;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};

void cleanup_i2s() {
    if (i2s_initialized) {
        i2s_stop(I2S_NUM_0);
        i2s_driver_uninstall(I2S_NUM_0);
        i2s_initialized = false;
        delay(100);
    }
}

void i2s_mic_init() {
    cleanup_i2s();
    
    // Configuration for SPH0645
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_MIC_SCK,
        .ws_io_num = I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SD
    };
    
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S driver for mic: %d\n", err);
        return;
    }
    
    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S pins for mic: %d\n", err);
        return;
    }
    
    i2s_initialized = true;
    delay(100);
}

void i2s_speaker_init() {
    cleanup_i2s();
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_SPKR_BCLK,
        .ws_io_num = I2S_SPKR_LRC,
        .data_out_num = I2S_SPKR_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S driver for speaker: %d\n", err);
        return;
    }
    
    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S pins for speaker: %d\n", err);
        return;
    }
    
    i2s_initialized = true;
    delay(100);
}

void writeWavHeader(File file, size_t dataSize) {
    WavHeader header;
    header.fileSize = dataSize + WAV_HEADER_SIZE - 8;
    header.dataSize = dataSize;
    file.write((const uint8_t*)&header, WAV_HEADER_SIZE);
}

void startRecording() {
    Serial.println("Starting recording...");
    
    i2s_mic_init();
    
    if (!i2s_initialized) {
        Serial.println("Failed to initialize I2S for recording");
        return;
    }
    
    if (SD.exists(RECORD_FILE)) {
        SD.remove(RECORD_FILE);
    }
    
    File file = SD.open(RECORD_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for recording");
        return;
    }
    
    // Write placeholder header
    WavHeader header;
    file.write((const uint8_t*)&header, WAV_HEADER_SIZE);
    
    size_t bytesWritten = 0;
    unsigned long startTime = millis();
    int32_t samples[BINARY_BUFFER_SIZE/4];
    
    Serial.println("Recording...");
    
    while ((millis() - startTime) < (RECORD_TIME * 1000)) {
        size_t bytesRead = 0;
        esp_err_t result = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead, portMAX_DELAY);
        
        if (result == ESP_OK && bytesRead > 0) {
            // Process SPH0645 data: 24-bit signed integer in MSB format
            int16_t converted[BINARY_BUFFER_SIZE/8];
            for (int i = 0; i < bytesRead/4; i++) {
                // Convert 24-bit to 16-bit with proper scaling
                converted[i] = (int16_t)(samples[i] >> 14);
            }
            file.write((const uint8_t*)converted, bytesRead/2);
            bytesWritten += bytesRead/2;
        }
    }
    
    // Update WAV header with final size
    file.seek(0);
    writeWavHeader(file, bytesWritten);
    file.close();
    
    cleanup_i2s();
    Serial.println("Recording finished!");
}

void playRecording() {
    Serial.println("Playing recording...");
    
    // Initialize speaker
    i2s_speaker_init();
    if (!i2s_initialized) {
        Serial.println("Failed to initialize I2S for playback");
        return;
    }
    
    // Open WAV file
    File file = SD.open(RECORD_FILE);
    if (!file) {
        Serial.println("Failed to open file for playback");
        return;
    }
    
    // Skip WAV header
    file.seek(WAV_HEADER_SIZE);
    
    // Read and play file
    uint8_t buffer[1024];
    size_t bytes_read;
    Serial.println("Starting playback...");
    
    while (file.available()) {
        bytes_read = file.read(buffer, sizeof(buffer));
        if (bytes_read > 0) {
            size_t bytes_written = 0;

            // Amplify the signal
            int16_t *samples = (int16_t*)buffer;
            for(int i=0; i < bytes_read/2; i++) {
              samples[i] = samples[i] * 4;  // Multiply by 2-8 for more volume
            }
            
            esp_err_t result = i2s_write(I2S_NUM_0, buffer, bytes_read, &bytes_written, portMAX_DELAY);
            if (result != ESP_OK) {
                Serial.printf("Failed to write I2S data: %d\n", result);
            }
        }
    }
    
    file.close();
    cleanup_i2s();
    Serial.println("Playback finished!");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("Initializing...");
    
    // Initialize SD card
    if (!SD.begin()) {
        Serial.println("Card Mount Failed");
        return;
    }
    
    // Make sure I2S is clean at startup
    cleanup_i2s();
    
    Serial.println("Ready! Send 'R' to start recording.");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'R' || cmd == 'r') {
            startRecording();
            delay(500);
            playRecording();
        }
    }
}