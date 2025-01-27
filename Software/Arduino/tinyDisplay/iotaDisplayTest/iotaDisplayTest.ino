#include <Arduino_GFX_Library.h>

#define TFT_MOSI MOSI
#define TFT_SCLK SCK
#define TFT_CS   SCL
#define TFT_DC   13
#define TFT_RST  12
#define TFT_BL   11

Arduino_DataBus *bus = nullptr;
Arduino_GFX *gfx = nullptr;

uint16_t getHueColor(float hue) {
  float r, g, b;
  float h = fmod(hue, 360);
  float s = 1.0f;
  float v = 1.0f;
  
  float c = v * s;
  float x = c * (1 - abs(fmod(h / 60.0f, 2) - 1));
  float m = v - c;
  
  if(h >= 0 && h < 60) { r = c; g = x; b = 0; }
  else if(h >= 60 && h < 120) { r = x; g = c; b = 0; }
  else if(h >= 120 && h < 180) { r = 0; g = c; b = x; }
  else if(h >= 180 && h < 240) { r = 0; g = x; b = c; }
  else if(h >= 240 && h < 300) { r = x; g = 0; b = c; }
  else { r = c; g = 0; b = x; }
  
  uint8_t r8 = (r + m) * 255;
  uint8_t g8 = (g + m) * 255;
  uint8_t b8 = (b + m) * 255;
  
  return ((r8 & 0xF8) << 8) | ((g8 & 0xFC) << 3) | (b8 >> 3);
}

void setup() {
  Serial.begin(115200);
  
  bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
  gfx = new Arduino_GC9A01(bus, TFT_RST, 0, true);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  if (!gfx->begin()) {
    Serial.println("Display initialization failed!");
    while(1) delay(100);
  }
  
  for(int y = 0; y < 240; y++) {
    for(int x = 0; x < 240; x++) {
      float progress = (x + y) / 480.0f;  // diagonal progress from 0 to 1
      uint16_t color = getHueColor(progress * 360.0f);  // map to 0-360 degrees
      gfx->drawPixel(x, y, color);
    }
  }
}

void loop() {
  delay(1000);
}