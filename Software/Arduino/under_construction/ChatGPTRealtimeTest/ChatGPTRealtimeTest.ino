#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include "mbedtls/base64.h"

// WiFi credentials
const char* ssid = "WIFINAME";
const char* password = "WIFIPASSWORD";

// OpenAI API configuration
const char* openai_api_key = "YOURAPIKEYHERE";

// Pin Definitions
#define I2S_BCLK      8
#define I2S_LRC       9
#define I2S_DOUT      10  // Data out from ESP32 to speaker
#define I2S_DIN       11  // Data in from mic to ESP32

// Constants
const int SAMPLE_RATE = 24000;
const int BUFFER_SIZE = 512;
const unsigned long RECONNECT_INTERVAL = 2000;
const unsigned long CONNECTION_TIMEOUT = 10000;
const unsigned long MODE_SWITCH_DELAY = 500;  // Delay between mode switches

// State enum
enum DeviceState {
    IDLE,
    LISTENING,
    PROCESSING,
    PLAYING,
    SWITCHING_MODE
};

// Global variables
WebSocketsClient webSocket;
bool isWebSocketConnected = false;
DeviceState currentState = IDLE;
i2s_port_t i2s_port = I2S_NUM_0;
unsigned long lastConnectionAttempt = 0;
unsigned long lastWebSocketActivity = 0;
unsigned long modeSwitchStartTime = 0;
String pendingAudioResponse = "";

// Function declarations
void setupI2S();
void configureI2SForMic();
void configureI2SForSpeaker();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handleServerMessage(const char * message, size_t length);
void sendSessionUpdate();
void processAudioInput();
void playAudioOutput(uint8_t* data, size_t length);
String base64Encode(uint8_t* data, size_t length);
uint8_t* base64Decode(const char* encodedData, size_t* outputLength);
void updateState() {
    unsigned long currentMillis = millis();

    switch (currentState) {
        case IDLE:
            // Nothing to do in idle state
            break;

        case SWITCHING_MODE:
            if (currentMillis - modeSwitchStartTime >= MODE_SWITCH_DELAY) {
                if (pendingAudioResponse.isEmpty()) {
                    // Switching to microphone mode
                    configureI2SForMic();
                    currentState = LISTENING;
                } else {
                    // Switching to speaker mode
                    configureI2SForSpeaker();
                    currentState = PLAYING;
                }
            }
            break;

        case LISTENING:
            if (isWebSocketConnected) {
                processAudioInput();
            }
            break;

        case PROCESSING:
            // Wait for response, handled in handleServerMessage
            break;

        case PLAYING:
            if (!pendingAudioResponse.isEmpty()) {
                size_t decodedLen;
                uint8_t* audioData = base64Decode(pendingAudioResponse.c_str(), &decodedLen);
                if (audioData != NULL) {
                    playAudioOutput(audioData, decodedLen);
                    free(audioData);
                    
                    // Clear the pending response
                    pendingAudioResponse = "";
                    
                    // Switch back to listening mode
                    currentState = SWITCHING_MODE;
                    modeSwitchStartTime = currentMillis;
                }
            }
            break;
    }
}

// Function declarations
void setupI2S();
void configureI2SForMic();
void configureI2SForSpeaker();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handleServerMessage(const char * message, size_t length);
void sendSessionUpdate();
void processAudioInput();
void playAudioOutput(uint8_t* data, size_t length);
String base64Encode(uint8_t* data, size_t length);
uint8_t* base64Decode(const char* encodedData, size_t* outputLength);

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Setup I2S
    setupI2S();
    
    // Configure WebSocket
    webSocket.setExtraHeaders(("Authorization: Bearer " + String(openai_api_key) + "\r\nOpenAI-Beta: realtime=v1").c_str());
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(RECONNECT_INTERVAL);
    webSocket.enableHeartbeat(15000, 3000, 2);  // Enable heartbeat to detect connection issues
    
    // Initial WebSocket connection
    webSocket.beginSSL("api.openai.com", 443, "/v1/realtime?model=gpt-4o-realtime-preview-2024-10-01");
    lastConnectionAttempt = millis();
    
    Serial.println("Ready! Press 'R' to start/stop recording.");
}

void loop() {
    // Handle WebSocket connection management
    unsigned long currentMillis = millis();
    
    // Check if we need to reconnect
    if (!isWebSocketConnected && (currentMillis - lastConnectionAttempt > RECONNECT_INTERVAL)) {
        Serial.println("Attempting to reconnect WebSocket...");
        webSocket.beginSSL("api.openai.com", 443, "/v1/realtime?model=gpt-4o-realtime-preview-2024-10-01");
        lastConnectionAttempt = currentMillis;
    }
    
    // Check for connection timeout
    if (isWebSocketConnected && (currentMillis - lastWebSocketActivity > CONNECTION_TIMEOUT)) {
        Serial.println("WebSocket connection timed out, forcing reconnection...");
        webSocket.disconnect();
        isWebSocketConnected = false;
        lastConnectionAttempt = currentMillis - RECONNECT_INTERVAL;
    }

    webSocket.loop();

    // Handle user commands
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'R' || cmd == 'r') {
            if (currentState == IDLE) {
                Serial.println("Starting listening...");
                currentState = SWITCHING_MODE;
                modeSwitchStartTime = currentMillis;
            } else if (currentState == LISTENING) {
                Serial.println("Stopping...");
                currentState = IDLE;
            }
        }
    }

    // Update state machine
    updateState();

    // Small delay to prevent watchdog reset
    delay(1);
}

void setupI2S() {
    Serial.println("Initializing I2S...");
    
    esp_err_t err;
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,        // Reduced buffer count
        .dma_buf_len = 512,        // Smaller buffer size
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,  // Same pin for input/output
        .data_in_num = I2S_DOUT    // Using same pin as data_out
    };

    // Install and configure I2S driver
    err = i2s_driver_install(i2s_port, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S driver: %d\n", err);
        return;
    }

    err = i2s_set_pin(i2s_port, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S pins: %d\n", err);
        return;
    }

    Serial.println("I2S initialized successfully");
}

void configureI2SForMic() {
    Serial.println("Configuring I2S for microphone...");
    esp_err_t err;
    
    err = i2s_stop(i2s_port);
    if (err != ESP_OK) {
        Serial.printf("Failed to stop I2S: %d\n", err);
        return;
    }
    
    err = i2s_set_clk(i2s_port, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_MONO);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S clock: %d\n", err);
        return;
    }
    
    err = i2s_start(i2s_port);
    if (err != ESP_OK) {
        Serial.printf("Failed to start I2S: %d\n", err);
        return;
    }
    
    err = i2s_zero_dma_buffer(i2s_port);
    if (err != ESP_OK) {
        Serial.printf("Failed to zero DMA buffer: %d\n", err);
        return;
    }
    
    Serial.println("Microphone configured successfully");

    // Flush any initial data
    uint8_t dummy[512];
    size_t bytes_read;
    for(int i = 0; i < 5; i++) {
        i2s_read(i2s_port, dummy, sizeof(dummy), &bytes_read, 0);
    }
}

void configureI2SForSpeaker() {
    i2s_stop(i2s_port);
    i2s_set_clk(i2s_port, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    i2s_start(i2s_port);
    i2s_zero_dma_buffer(i2s_port);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    lastWebSocketActivity = millis();  // Update activity timestamp
    
    switch(type) {
        case WStype_DISCONNECTED:
            if (isWebSocketConnected) {  // Only print if we were previously connected
                Serial.println("WebSocket disconnected");
            }
            isWebSocketConnected = false;
            break;
            
        case WStype_CONNECTED:
            isWebSocketConnected = true;
            Serial.println("WebSocket connected");
            sendSessionUpdate();
            break;
            
        case WStype_TEXT:
            handleServerMessage((const char*)payload, length);
            break;
            
        case WStype_ERROR:
            Serial.println("WebSocket error");
            break;
            
        case WStype_PING:
            lastWebSocketActivity = millis();  // Update on ping
            break;
            
        case WStype_PONG:
            lastWebSocketActivity = millis();  // Update on pong
            break;
    }
}

void sendSessionUpdate() {
    StaticJsonDocument<1024> doc;
    doc["type"] = "session.update";
    JsonObject session = doc.createNestedObject("session");
    
    session["voice"] = "alloy";
    session["instructions"] = "You are a helpful AI assistant.";
    
    JsonObject turnDetection = session.createNestedObject("turn_detection");
    turnDetection["type"] = "server_vad";
    turnDetection["threshold"] = 0.5;
    turnDetection["prefix_padding_ms"] = 300;
    turnDetection["silence_duration_ms"] = 200;
    
    JsonObject transcription = session.createNestedObject("input_audio_transcription");
    transcription["model"] = "whisper-1";
    
    session["input_audio_format"] = "pcm16";
    session["output_audio_format"] = "pcm16";

    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.sendTXT(jsonString);
}

void processAudioInput() {
    static uint8_t audioBuffer[BUFFER_SIZE];
    size_t bytesRead = 0;
    static unsigned long lastAudioSend = 0;
    const unsigned long AUDIO_SEND_INTERVAL = 20;  // Send every 20ms
    
    unsigned long currentTime = millis();
    if (currentTime - lastAudioSend < AUDIO_SEND_INTERVAL) {
        return;  // Don't send too frequently
    }
    
    esp_err_t err = i2s_read(i2s_port, audioBuffer, BUFFER_SIZE, &bytesRead, 0);
    if (err != ESP_OK) {
        Serial.printf("I2S read error: %d\n", err);
        return;
    }
    
    if (bytesRead > 0) {
        // Convert 32-bit samples to 16-bit
        int32_t* samples32 = (int32_t*)audioBuffer;
        int16_t samples16[BUFFER_SIZE/4];
        
        // Track audio levels for debugging
        float maxLevel = 0;
        
        for (int i = 0; i < bytesRead/4; i++) {
            // Right-shift by 14 bits for SPH0645
            int32_t sample = samples32[i] >> 14;
            samples16[i] = (int16_t)(sample >> 2);  // Further shift to 16-bit
            
            // Track maximum level
            float level = abs((float)samples16[i] / 32768.0f);
            if (level > maxLevel) maxLevel = level;
        }
        
        // Print audio level meter every 500ms
        static unsigned long lastLevelPrint = 0;
        if (currentTime - lastLevelPrint > 500) {
            int barLength = (int)(maxLevel * 30);
            Serial.print("Audio Level: [");
            for (int i = 0; i < 30; i++) {
                Serial.print(i < barLength ? "=" : " ");
            }
            Serial.println("]");
            lastLevelPrint = currentTime;
        }
        
        // Only send if we have actual audio
        if (maxLevel > 0.01) {  // Threshold to avoid sending silence
            if (!isWebSocketConnected) {
                Serial.println("WebSocket disconnected, can't send audio");
                return;
            }
            
            String encodedAudio = base64Encode((uint8_t*)samples16, bytesRead/2);
            
            StaticJsonDocument<8192> doc;
            doc["type"] = "input_audio_buffer.append";
            doc["audio"] = encodedAudio;

            String jsonString;
            serializeJson(doc, jsonString);
            
            if (webSocket.sendTXT(jsonString)) {
                lastWebSocketActivity = millis();
                lastAudioSend = currentTime;
            } else {
                Serial.println("Failed to send audio data");
            }
        }
    }
}

void handleServerMessage(const char * message, size_t length) {
    StaticJsonDocument<8192> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) return;

    const char* type = doc["type"];
    
    if (strcmp(type, "response.audio_transcript.delta") == 0) {
        if (const char* delta = doc["delta"]) {
            Serial.print(delta);
        }
    }
    else if (strcmp(type, "response.audio.delta") == 0) {
        if (const char* audioDelta = doc["delta"]) {
            // Store the audio response
            pendingAudioResponse = audioDelta;
            currentState = PLAYING;
        }
    }
    else if (strcmp(type, "error") == 0) {
        Serial.print("Error: ");
        Serial.println(doc["error"]["message"].as<const char*>());
    }
}

void playAudioOutput(uint8_t* data, size_t length) {
    // Apply simple audio processing
    int16_t* samples = (int16_t*)data;
    size_t numSamples = length / 2;
    
    const float threshold = 500;
    const float gain = 1.5;
    
    for (size_t i = 0; i < numSamples; i++) {
        float sample = samples[i];
        if (abs(sample) < threshold) {
            sample = 0;
        } else {
            sample *= gain;
            if (sample > 32767) sample = 32767;
            if (sample < -32767) sample = -32767;
        }
        samples[i] = (int16_t)sample;
    }
    
    size_t bytesWritten = 0;
    i2s_write(i2s_port, data, length, &bytesWritten, portMAX_DELAY);
}

String base64Encode(uint8_t* data, size_t length) {
    size_t encodedLen = 4 * ((length + 2) / 3);
    unsigned char* encodedData = (unsigned char*)malloc(encodedLen + 1);
    
    size_t outputLen = 0;
    int ret = mbedtls_base64_encode(encodedData, encodedLen + 1, &outputLen, data, length);
    
    String result;
    if (ret == 0) {
        encodedData[outputLen] = '\0';
        result = String((char*)encodedData);
    }
    
    free(encodedData);
    return result;
}

uint8_t* base64Decode(const char* encodedData, size_t* outputLength) {
    size_t inputLen = strlen(encodedData);
    size_t decodedLen = (inputLen * 3) / 4 + 1;
    uint8_t* decodedData = (uint8_t*)malloc(decodedLen);
    
    int ret = mbedtls_base64_decode(decodedData, decodedLen, outputLength, 
                                   (const unsigned char*)encodedData, inputLen);
    
    if (ret == 0) {
        return decodedData;
    }
    
    free(decodedData);
    return NULL;
}