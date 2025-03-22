#include <Adafruit_MCP4728.h>
#include <Wire.h>
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"

Adafruit_MCP4728 mcp;

// Constants
const int ANALOG_PIN = A0;  // GPIO18, ADC2_CH7
const unsigned long LIGHT_CHANGE_INTERVAL = 10000;  // 10ms in microseconds
const int LED_VALUE = 100;  // LED brightness value
const unsigned long SAMPLE_INTERVAL = 25;  // 25 microseconds

// Variables for timing
unsigned long lastSampleMicros = 0;
unsigned long lastLightMicros = 0;
uint8_t currentLight = 0;
volatile uint16_t analogBuffer[100];  // Buffer for analog readings
volatile uint8_t bufferIndex = 0;
volatile bool bufferFull = false;

// ADC handles
adc_oneshot_unit_handle_t adc2_handle;
adc_oneshot_unit_init_cfg_t init_config2;
adc_oneshot_chan_cfg_t config;

void setup(void) {
  Serial.begin(2000000);  // Increased baud rate for faster data transmission
  
  // Initialize I2C and LED control
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);

  Wire.begin(3, 4);
  Wire.setClock(800000);  // Set I2C clock to 800kHz for faster communication
  delay(100);  // Give I2C time to stabilize

  // Try to initialize MCP4728
  if (!mcp.begin()) {
    Serial.println("Failed to find MCP4728 chip");
    while (1) {
      delay(10);
    }
  }

  // Configure ADC
  init_config2.unit_id = ADC_UNIT_2;  // Using ADC2
  init_config2.ulp_mode = ADC_ULP_MODE_DISABLE;
  init_config2.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc2_handle));

  // Configure ADC channel
  config.atten = ADC_ATTEN_DB_11;
  config.bitwidth = ADC_BITWIDTH_12;
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_7, &config));  // ADC2_CH7 for GPIO18

  // Initialize all channels to 0
  mcp.setChannelValue(MCP4728_CHANNEL_A, 0);
  mcp.setChannelValue(MCP4728_CHANNEL_B, 0);
  mcp.setChannelValue(MCP4728_CHANNEL_C, 0);
  mcp.setChannelValue(MCP4728_CHANNEL_D, 0);

  // Print header for Serial Plotter
  Serial.println("Light_Level Channel_A Channel_B Channel_C Channel_D");
}

void loop() {
  unsigned long currentMicros = micros();
  
  // Sample analog input every 50 microseconds
  if (currentMicros - lastSampleMicros >= SAMPLE_INTERVAL) {
    lastSampleMicros = currentMicros;
    
    // Read ADC
    int adc_value;
    if (adc_oneshot_read(adc2_handle, ADC_CHANNEL_7, &adc_value) == ESP_OK) {
      analogBuffer[bufferIndex] = adc_value;
      
      // Print the values and LED states
      Serial.println(analogBuffer[bufferIndex]);
      // Serial.print(" ");
      // Serial.print(currentLight == 0 ? LED_VALUE : 0);
      // Serial.print(" ");
      // Serial.print(currentLight == 1 ? LED_VALUE : 0);
      // Serial.print(" ");
      // Serial.print(currentLight == 2 ? LED_VALUE : 0);
      // Serial.print(" ");
      // Serial.println(currentLight == 3 ? LED_VALUE : 0);
      
      bufferIndex = (bufferIndex + 1) % 100;
      if (bufferIndex == 0) {
        bufferFull = true;
      }
    }
  }

  // Change lights every 10ms (10,000 microseconds)
  if (currentMicros - lastLightMicros >= LIGHT_CHANGE_INTERVAL) {
    lastLightMicros = currentMicros;
    
    // Turn off all LEDs
    mcp.setChannelValue(MCP4728_CHANNEL_A, 0);
    mcp.setChannelValue(MCP4728_CHANNEL_B, 0);
    mcp.setChannelValue(MCP4728_CHANNEL_C, 0);
    mcp.setChannelValue(MCP4728_CHANNEL_D, 0);

    // If we're not in the "all off" state, turn on the current LED
    if (currentLight < 4) {
      switch(currentLight) {
        case 0:
          mcp.setChannelValue(MCP4728_CHANNEL_A, LED_VALUE);
          break;
        case 1:
          mcp.setChannelValue(MCP4728_CHANNEL_B, LED_VALUE);
          break;
        case 2:
          mcp.setChannelValue(MCP4728_CHANNEL_C, LED_VALUE);
          break;
        case 3:
          mcp.setChannelValue(MCP4728_CHANNEL_D, LED_VALUE);
          break;
      }
    }

    // Increment light counter
    currentLight = (currentLight + 1) % 5;  // 5 states: 4 LEDs + all off
  }
}