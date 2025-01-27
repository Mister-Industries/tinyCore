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
// Microphone (SPH0645)
#define I2S_MIC_SCK     8
#define I2S_MIC_WS      9
#define I2S_MIC_SD      10

// Speaker
#define I2S_SPKR_BCLK   8
#define I2S_SPKR_LRC    9
#define I2S_SPKR_DIN    10

// Global flag to track I2S state
bool i2s_initialized = false;

WebSocketsClient webSocket;
bool isWebSocketConnected = false;
bool isRecording = false;

// Function declarations
void cleanup_i2s();
void i2s_mic_init();
void i2s_speaker_init();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handleServerMessage(const char * message, size_t length);
void sendSessionUpdate();
void readAndSendAudio();
void playAudioData(uint8_t * data, size_t length);
String base64Encode(uint8_t * data, size_t length);
uint8_t * base64Decode(const char * encodedData, size_t * outputLength);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("Initializing...");

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");

    // Test the speaker
    testSpeaker();

    // Initialize WebSocket client
    webSocket.beginSSL("api.openai.com", 443, "/v1/realtime?model=gpt-4o-realtime-preview-2024-10-01");

    // Set required headers
    String headers = "Authorization: Bearer " + String(openai_api_key) + "\r\n"
                     "OpenAI-Beta: realtime=v1";
    webSocket.setExtraHeaders(headers.c_str());

    // Set event handler
    webSocket.onEvent(webSocketEvent);

    Serial.println("Ready! Press 'R' to start interaction.");
}

void loop() {
    webSocket.loop();

    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'R' || cmd == 'r') {
            Serial.println("Starting interaction...");
            isRecording = true;
            // Initialize microphone
            i2s_mic_init();
        }
    }

    if(isWebSocketConnected && isRecording) {
        // Read and send audio data
        readAndSendAudio();
    }

    delay(10); // Small delay to avoid overloading the CPU
}

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
        .sample_rate = 24000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // The SPH0645 sends 32-bit data
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // Mono mic
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,    // Increased buffer count
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

    i2s_zero_dma_buffer(I2S_NUM_0);

    i2s_initialized = true;
    delay(100);
}

void i2s_speaker_init() {
    cleanup_i2s();

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 24000,
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

    i2s_zero_dma_buffer(I2S_NUM_0);
    i2s_initialized = true;
    Serial.println("Speaker initialized successfully");
}

void testSpeaker() {
    Serial.println("Testing speaker with a beep...");
    cleanup_i2s();
    i2s_speaker_init();
    
    // Generate a simple sine wave beep
    const int sampleRate = 24000;
    const float frequency = 440;  // 440 Hz (A4 note)
    const float amplitude = 20000;  // Reduced amplitude
    const int duration = 500;     // Longer duration (500ms)
    const int numSamples = (sampleRate * duration) / 1000;
    
    int16_t* samples = (int16_t*)malloc(numSamples * sizeof(int16_t));
    if (samples == NULL) {
        Serial.println("Failed to allocate memory for beep");
        return;
    }

    // Apply a smooth envelope to reduce clicks
    const int rampSamples = sampleRate / 100;  // 10ms ramp
    
    for(int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        float envelope = 1.0;
        
        // Apply fade in
        if(i < rampSamples) {
            envelope = (float)i / rampSamples;
        }
        // Apply fade out
        else if(i > numSamples - rampSamples) {
            envelope = (float)(numSamples - i) / rampSamples;
        }
        
        samples[i] = (int16_t)(amplitude * envelope * sin(2 * PI * frequency * t));
    }

    // Play the beep
    size_t bytesWritten = 0;
    esp_err_t result = i2s_write(I2S_NUM_0, samples, numSamples * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    
    if (result != ESP_OK) {
        Serial.printf("Failed to write I2S data: %d\n", result);
    }

    free(samples);
    delay(duration + 100);  // Wait for the beep to finish
    
    cleanup_i2s();
    i2s_mic_init();
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("\n[WebSocket] Disconnected");
            isWebSocketConnected = false;
            break;
        case WStype_CONNECTED:
            Serial.println("\n[WebSocket] Connected");
            isWebSocketConnected = true;
            sendSessionUpdate();
            break;
        case WStype_TEXT: {
            //Serial.println("\n[WebSocket] Received text:");
            // Print the raw message
            String msg((char*)payload);
            //Serial.println(msg);
            
            // Try to parse as JSON for better debug visibility
            StaticJsonDocument<8192> doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
                const char* msgType = doc["type"];
                //Serial.printf("Message Type: %s\n", msgType);
                
                // If it's a transcription, show it
                if (doc.containsKey("transcription")) {
                    Serial.print("Transcribed: ");
                    Serial.println(doc["transcription"].as<const char*>());
                }
            }
            
            handleServerMessage((const char*)payload, length);
            break;
        }
        case WStype_ERROR:
            Serial.printf("\n[WebSocket] Error: %s\n", payload);
            break;
        case WStype_PING:
            Serial.println("\n[WebSocket] Received ping");
            break;
        case WStype_PONG:
            Serial.println("\n[WebSocket] Received pong");
            break;
        default:
            Serial.printf("\n[WebSocket] Received unknown type: %d\n", type);
            break;
    }
}

void sendSessionUpdate() {
    StaticJsonDocument<1024> doc;
    doc["type"] = "session.update";
    JsonObject session = doc.createNestedObject("session");
    
    // Basic settings
    session["voice"] = "alloy";
    session["instructions"] = "Your knowledge cutoff is 2023-10. You are a helpful AI assistant.";

    // Turn detection settings
    JsonObject turnDetection = session.createNestedObject("turn_detection");
    turnDetection["type"] = "server_vad";
    turnDetection["threshold"] = 0.5;
    turnDetection["prefix_padding_ms"] = 300;
    turnDetection["silence_duration_ms"] = 200;

    // Audio transcription - just specify the model without the enable flag
    JsonObject transcription = session.createNestedObject("input_audio_transcription");
    transcription["model"] = "whisper-1";

    // Audio format settings
    session["input_audio_format"] = "pcm16";
    session["output_audio_format"] = "pcm16";

    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println("Sending configuration:");
    Serial.println(jsonString);
    webSocket.sendTXT(jsonString);
}

void handleServerMessage(const char * message, size_t length) {
    StaticJsonDocument<8192> doc;
    DeserializationError error = deserializeJson(doc, message);
    if(error) {
        return;
    }

    const char* type = doc["type"];
    
    // Only show transcripts and errors, with minimal formatting
    if(strcmp(type, "response.audio_transcript.delta") == 0) {
        const char* delta = doc["delta"];
        if(delta) {
            // Print without newline so words connect naturally
            Serial.print(delta);
        }
    }
    else if(strcmp(type, "error") == 0) {
        JsonObject errorObj = doc["error"];
        const char* message = errorObj["message"];
        Serial.printf("\nError: %s\n", message);
    }
    else if(strcmp(type, "response.audio.delta") == 0) {
        const char* audioDelta = doc["delta"];
        if(audioDelta != nullptr) {
            size_t decodedLen;
            uint8_t * audioData = base64Decode(audioDelta, &decodedLen);
            if(audioData != NULL) {
                isRecording = false;
                cleanup_i2s();
                i2s_speaker_init();
                playAudioData(audioData, decodedLen);
                free(audioData);
                cleanup_i2s();
                i2s_mic_init();
                isRecording = true;
            }
        }
    }
}

void readAndSendAudio() {
    const int audioBufSize = 1024;
    uint8_t rawData[audioBufSize];
    size_t bytesRead = 0;

    static unsigned long lastDebugTime = 0;
    static unsigned long sampleCount = 0;
    static float maxAmplitude = 0;
    static unsigned long lastSendTime = 0;
    unsigned long currentTime = millis();

    // Read raw data from I2S mic
    esp_err_t result = i2s_read(I2S_NUM_0, rawData, audioBufSize, &bytesRead, portMAX_DELAY);

    if (result == ESP_OK && bytesRead > 0) {
        // Convert and analyze audio data
        int32_t *samples_32 = (int32_t *)rawData;
        int16_t samples_16[audioBufSize / 2];
        
        // Track maximum amplitude for this batch
        float maxSample = 0;
        
        // Process each sample
        for (int i = 0; i < bytesRead / 4; i++) {
            // The SPH0645 sends data left-aligned, we need to shift right by 14 bits
            // to get the 18-bit sample value
            int32_t sample = samples_32[i] >> 14;
            
            // Convert to 16-bit
            samples_16[i] = (int16_t)(sample >> 2);
            
            // Calculate amplitude (absolute value)
            float amplitude = abs((float)samples_16[i]) / 32768.0f;  // Normalize to 0-1
            if (amplitude > maxSample) {
                maxSample = amplitude;
            }
        }

        // Update max amplitude
        if (maxSample > maxAmplitude) {
            maxAmplitude = maxSample;
        }

        sampleCount++;

        // Print debug info every second
        /*currentTime = millis();
        if (currentTime - lastDebugTime >= 1000) {  // Every second
            Serial.println("\n=== Audio Debug Info ===");
            Serial.printf("Samples processed: %lu\n", sampleCount);
            Serial.printf("Max amplitude: %.6f\n", maxAmplitude);
            Serial.printf("Current batch max: %.6f\n", maxSample);
            
            // Print a simple VU meter
            Serial.print("Level: [");
            int level = (int)(maxSample * 50);
            for (int i = 0; i < 50; i++) {
                Serial.print(i < level ? '#' : '-');
            }
            Serial.println("]");
            
            // Print some sample values for debugging
            Serial.println("Sample values (first 5):");
            for (int i = 0; i < 5; i++) {
                Serial.printf("%d ", samples_16[i]);
            }
            Serial.println();
            
            lastDebugTime = currentTime;
            maxAmplitude = 0;  // Reset for next second
            sampleCount = 0;
        }*/

        // Send the audio data
        size_t pcmBytes = (bytesRead / 4) * 2;  // Convert number of 32-bit samples to bytes of 16-bit samples
        String encodedAudio = base64Encode((uint8_t *)samples_16, pcmBytes);

        StaticJsonDocument<8192> doc;
        doc["type"] = "input_audio_buffer.append";
        doc["audio"] = encodedAudio;

        String jsonString;
        serializeJson(doc, jsonString);
        
        if (isWebSocketConnected) {
            bool success = webSocket.sendTXT(jsonString);
            currentTime = millis();
            if (success) {
                //Serial.printf("Sent audio data at %lu ms (interval: %lu ms)\n", 
                    //currentTime, currentTime - lastSendTime);
            } else {
                Serial.println("Failed to send audio data");
            }
            lastSendTime = currentTime;
        } else {
            Serial.println("WebSocket not connected, can't send audio");
        }
    } else if (result != ESP_OK) {
        Serial.printf("Error reading from I2S: %d\n", result);
    }
}

void playAudioData(uint8_t * data, size_t length) {
    // Convert data to 16-bit samples
    int16_t *samples = (int16_t*)data;
    size_t numSamples = length / 2;  // Since each sample is 2 bytes
    
    // Apply a simple noise gate and amplification
    const float threshold = 500;  // Adjust this threshold as needed
    const float gain = 2.0;      // Amplification factor
    
    for(size_t i = 0; i < numSamples; i++) {
        float sample = samples[i];
        
        // Apply noise gate
        if(abs(sample) < threshold) {
            sample = 0;
        } else {
            // Apply gain
            sample *= gain;
            // Clamp to prevent overflow
            if(sample > 32767) sample = 32767;
            if(sample < -32767) sample = -32767;
        }
        
        samples[i] = (int16_t)sample;
    }

    size_t bytesWritten = 0;
    esp_err_t result = i2s_write(I2S_NUM_0, data, length, &bytesWritten, portMAX_DELAY);
    
    if (result != ESP_OK) {
        Serial.printf("Failed to write I2S data: %d\n", result);
    }
}

String base64Encode(uint8_t * data, size_t length) {
    size_t encodedLen = 4 * ((length + 2) / 3);
    unsigned char * encodedData = (unsigned char *)malloc(encodedLen + 1);

    size_t outputLen = 0;
    int ret = mbedtls_base64_encode(encodedData, encodedLen + 1, &outputLen, data, length);
    if(ret == 0) {
        encodedData[outputLen] = '\0';
        String encodedString = String((char *)encodedData);
        free(encodedData);
        return encodedString;
    } else {
        free(encodedData);
        return "";
    }
}

uint8_t * base64Decode(const char * encodedData, size_t * outputLength) {
    size_t inputLen = strlen(encodedData);
    size_t decodedLen = (inputLen * 3) / 4 + 1; // Adjusted size
    uint8_t * decodedData = (uint8_t *)malloc(decodedLen);

    int ret = mbedtls_base64_decode(decodedData, decodedLen, outputLength, (const unsigned char *)encodedData, inputLen);
    if(ret == 0) {
        return decodedData;
    } else {
        free(decodedData);
        return NULL;
    }
}