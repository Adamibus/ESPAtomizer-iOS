// oled.h - consolidated OLED helper (single-file)
#ifndef OLED_H
#define OLED_H

#include "config.h"
#include "StateManager.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern GlobalState gState;

// Use internal linkage to avoid duplicate symbol issues when included
// from the single main sketch translation unit.
// Single definition provided in the main translation unit; header exposes externs.
extern Adafruit_SSD1306 display;

static bool tryInitOLED(int sdaPin, int sclPin) {
  Serial.printf("[OLED] Basic I2C check using SDA=%d SCL=%d\n", sdaPin, sclPin);
  pinMode(sdaPin, INPUT);
  pinMode(sclPin, INPUT);
  delay(5);
  int sdaLevel = digitalRead(sdaPin);
  int sclLevel = digitalRead(sclPin);
  Serial.printf("[OLED] SDA idle=%d SCL idle=%d\n", sdaLevel, sclLevel);

  Wire.begin(sdaPin, sclPin);
  Wire.setClock(100000UL);

  const uint8_t addrs[] = { 0x3C, 0x3D, 0x43 };
  for (size_t i = 0; i < sizeof(addrs)/sizeof(addrs[0]); ++i) {
    uint8_t a = addrs[i];
    Serial.printf("[OLED] Probing I2C addr 0x%02X...\n", a);
    Wire.beginTransmission(a);
    int err = Wire.endTransmission();
    Serial.printf("[OLED] addr 0x%02X endTransmission() => err=%d\n", a, err);
    if (err == 0) {
      Serial.printf("[OLED] Found device ACK at 0x%02X\n", a);
      bool began = false;
      #if defined(SSD1306_SWITCHCAPVCC)
        began = display.begin(SSD1306_SWITCHCAPVCC, a);
      #else
        began = display.begin(a);
      #endif
      Serial.printf("[OLED] display.begin(0x%02X) => %s\n", a, began?"OK":"FAIL");
      if (began) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print(F("OK"));
        display.display();
        gState.display.available = true;
        return true;
      }
    }
  }

  Serial.println(F("[OLED] No I2C ACK found on common addresses"));
  gState.display.available = false;
  return false;
}

#endif // OLED_H
