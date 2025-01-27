#include <driver/i2s.h>

// I2S pins for ESP32-S3 Feather
#define I2S_BCLK_PIN       8  // Bit clock
#define I2S_LRCLK_PIN      9  // Left/Right clock (Word Select)
#define I2S_DATA_PIN       10  // Data pin

// I2S configuration
#define SAMPLE_RATE     44100  // Audio sample rate in Hz
#define BITS_PER_SAMPLE    16  // Bits per sample
#define CHANNEL_NUM         2  // Number of channels (stereo)

// DMA buffer parameters
#define DMA_BUF_COUNT      8  // Number of DMA buffers
#define DMA_BUF_LEN      256  // Size of each DMA buffer

void setup() {
    Serial.begin(115200);
    
    // Configure I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // Configure I2S pins
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_LRCLK_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    // Install and start I2S driver
    esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.println("Error installing I2S driver");
        return;
    }
    
    result = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (result != ESP_OK) {
        Serial.println("Error setting I2S pins");
        return;
    }
    
    Serial.println("I2S initialized successfully");
}

// Example function to write a sine wave to I2S
void writeSineWave() {
    // Generate a 440 Hz sine wave
    const float frequency = 440.0;  // A4 note
    const float amplitude = 32000;  // Volume (max is 32767 for 16-bit)
    
    // Buffer for audio samples
    int16_t samples[DMA_BUF_LEN * 2];  // *2 for stereo
    
    static float phase = 0.0;
    const float phase_increment = 2.0 * PI * frequency / SAMPLE_RATE;
    
    // Fill the buffer with sine wave samples
    for (int i = 0; i < DMA_BUF_LEN * 2; i += 2) {
        int16_t sample = (int16_t)(amplitude * sin(phase));
        samples[i] = sample;      // Left channel
        samples[i + 1] = sample;  // Right channel
        
        phase += phase_increment;
        if (phase >= 2.0 * PI) {
            phase -= 2.0 * PI;
        }
    }
    
    // Write the samples to I2S
    size_t bytes_written;
    i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
}

void loop() {
    // Continuously output the sine wave
    writeSineWave();
}