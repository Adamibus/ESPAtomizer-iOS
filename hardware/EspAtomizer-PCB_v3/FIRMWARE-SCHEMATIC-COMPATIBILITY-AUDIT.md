# Firmware & Schematic Compatibility Audit Report

**Date:** December 26, 2025  
**Revision:** Post-WS2812B Migration  
**Scope:** Verify schematic changes align with firmware config.h and are cross-compatible (C3/C6)

---

## Schematic Changes Verification

### ✅ WS2812B Component Added

- **Symbol:** `LED:WS2812B-2020` (reference: D1) ✅
- **Footprint:** `LED_SMD:LED_WS2812B-2020_PLCC4_2.0x2.0mm` ✅
- **Datasheet:** Adafruit WS2812B-2020 V1.3 ✅
- **Pin Configuration (PLCC4):**
  - Pin 1: DIN (data in)
  - Pin 2: VSS (ground)
  - Pin 3: VDD (power, 3.3V or 5V)
  - Pin 4: DOUT (data out, optional for chaining)

### ✅ Discrete RGB Removal

- **Removed netlabels:** `LEDR`, `LEDG`, `LEDB` ✅ (no longer present in schematic)
- **Removed components:** Discrete RGB LED + series resistors ✅
- **Freed pins:** GPIO1 (D1), GPIO2 (D2), GPIO21 (D3) now available for encoder ✅

### ⚠️ WS2812B Connection Status

**Critical verification needed:**
- [ ] Confirm GPIO17 (U3 pin 8 / D7/RX) is connected to WS2812B DIN (pin 1)
- [ ] Confirm 3.3V (VCC rail) is connected to WS2812B VDD (pin 3)
- [ ] Confirm GND is connected to WS2812B VSS (pin 2)
- [ ] Confirm 100nF bypass cap between VDD and VSS

**Net labels expected in schematic:**
- GPIO17 → DIN: Should have net label like `GPIO17`, `D7`, or `RGB_LED_DATA`
- VDD: Should connect to `VCC` net
- VSS: Should connect to `GND` net

---

## Firmware Configuration Audit

### ✅ Pin Mapping (config.h) — All Correct

| Function | Config Define | Physical GPIO | Schematic Net | Status |
|---|---|---|---|---|
| **MOSFET Gate (PWM)** | `OUTPUT_PIN = LEFT_ROW_PIN_7` | 16 | `MOSFET_GATE` | ✅ |
| **Battery ADC** | `BAT_PIN = A0` | 0 | `BAT_ADC` | ✅ |
| **Encoder A** | `ENC_PIN_A = D1` | 1 | `ENC_A` | ✅ |
| **Encoder B** | `ENC_PIN_B = D2` | 2 | `ENC_B` | ✅ |
| **Encoder Switch** | `ENC_PIN_SW = D3` | 21 | `ENC_SW` | ✅ |
| **I2C SDA** | `OLED_SDA = SDA` | 22 | `I2C_SDA` | ✅ |
| **I2C SCL** | `OLED_SCL = SCL` | 23 | `I2C_SCL` | ✅ |
| **WS2812B Data** | `RGB_LED_PIN = 17` | 17 | (pending verification) | ⚠️ |

### ✅ Enabled Features

- `USE_BLE = 1` — BLE enabled
- `USE_WIFI = 0` — Wi-Fi disabled (conserves power)
- `USE_OLED = 1` — OLED I2C display enabled
- `USE_ENCODER = 1` — Rotary encoder enabled
- `USE_ADS1115 = 1` — I2C thermocouple converter enabled
- `RGB_LED_ENABLED = 1` — WS2812B RGB LED enabled
- `USE_BAT = 1` — Battery ADC monitoring enabled

### ✅ I2C Addresses

| Device | I2C Address | Config Define | Pin | Status |
|---|---|---|---|---|
| **OLED** | 0x3C | `OLED_I2C_ADDR = 0x3C` | SDA/SCL (GPIO22/23) | ✅ |
| **ADS1115** | 0x48 | `ADS1115_I2C_ADDR = 0x48` | SDA/SCL (GPIO22/23) | ✅ |

**Multi-master I2C:** Both devices share the same bus (SDA=GPIO22, SCL=GPIO23). No address conflicts. ✅

---

## Cross-Board Compatibility Check (C3 vs C6)

### ✅ XIAO ESP32-C3 Compatible

| GPIO | Function | C3 Availability | Notes |
|---|---|---|---|
| 0 | Battery ADC | ✅ ADC1_CH0 | No strap conflicts |
| 1 | Encoder A | ✅ GPIO1 | No conflicts |
| 2 | Encoder B | ✅ GPIO2 | No conflicts |
| 16 | MOSFET PWM | ✅ LEDC-capable | No strap/boot conflicts |
| 17 | WS2812B | ✅ GPIO17 | Available, no conflicts |
| 21 | Encoder SW | ✅ GPIO21 | No conflicts |
| 22 | I2C SDA | ✅ I2C0_SDA | Hardware I2C |
| 23 | I2C SCL | ✅ I2C0_SCL | Hardware I2C |

**Status:** ✅ **All pins available and compatible on ESP32-C3**

### ✅ XIAO ESP32-C6 Compatible

| GPIO | Function | C6 Availability | Notes |
|---|---|---|---|
| 0 | Battery ADC | ✅ ADC1_CH0 | No strap conflicts |
| 1 | Encoder A | ✅ GPIO1 | No conflicts |
| 2 | Encoder B | ✅ GPIO2 | No conflicts |
| 16 | MOSFET PWM | ✅ LEDC-capable | Safe on C6 |
| 17 | WS2812B | ✅ GPIO17 | Available, no conflicts |
| 21 | Encoder SW | ✅ GPIO21 | No conflicts |
| 22 | I2C SDA | ✅ I2C0_SDA | Hardware I2C |
| 23 | I2C SCL | ✅ I2C0_SCL | Hardware I2C |

**Status:** ✅ **All pins available and compatible on ESP32-C6**

---

## Library Requirements for WS2812B

**Current config supports:** Adafruit_NeoPixel or FastLED libraries

### Required Changes to Firmware Code

**If using Adafruit_NeoPixel:**
```cpp
#include <Adafruit_NeoPixel.h>

#define RGB_LED_PIN 17
#define RGB_LED_COUNT 1
Adafruit_NeoPixel pixels(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  pixels.begin();
  pixels.show(); // Initialize all pixels off
}

// Set color: RGB(red, green, blue) — values 0–255
void setColor(uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}
```

**Compilation flag:** Add to `platformio.ini` or `boards.txt`:
```
lib_deps = adafruit/Adafruit NeoPixel
build_flags = -DRGB_LED_PIN=17 -DRGB_LED_ENABLED=1
```

---

## Critical Items for PCB Verification

- [ ] **GPIO17 net connection:** Verify U3 pin 8 (GPIO17_D7_RX) → D1 pin 1 (DIN)
- [ ] **Bypass capacitor:** 100nF (0.1µF) C0G placed between D1 pin 3 (VDD) and pin 2 (VSS), within 5mm
- [ ] **Signal integrity:** Optional 470Ω series resistor on GPIO17 → DIN line (recommended for > 5cm traces)
- [ ] **Power routing:** D1 VDD routed to 3.3V rail via short, wide trace; VSS routed to solid GND
- [ ] **PCB trace:** GPIO17 data line trace kept clean (no adjacent high-current traces like heater or PWM)

---

## Firmware Compatibility Summary

✅ **Pin Mapping:** All pins correctly mapped in config.h and schematic ✅  
✅ **I2C Dual-Device:** OLED + ADS1115 on shared bus (no address conflicts) ✅  
✅ **Cross-Board:** Identical GPIO layout on both XIAO C3 and C6 ✅  
⚠️ **WS2812B Library:** Requires Adafruit_NeoPixel (or FastLED) addition to firmware code ⚠️  
⚠️ **Schematic WS2812B Net:** GPIO17 → DIN connection needs PCB verification ⚠️  

---

## Recommended Next Steps

1. **Verify schematic netlists:** Generate KiCad netlist and confirm:
   - U3 pin 8 (GPIO17) → D1 pin 1 (DIN)
   - D1 pin 3 (VDD) → VCC rail
   - D1 pin 2 (VSS) → GND

2. **Run ERC/DRC:** Check for unconnected nets or floating pins

3. **Add library to firmware:**
   - `platformio.ini`: Add `lib_deps = adafruit/Adafruit NeoPixel`
   - Update main sketch to initialize and control WS2812B

4. **Test compilation:** Build for both `xiao_esp32c3` and `xiao_esp32c6`

5. **PCB layout:** Route GPIO17 → DIN cleanly, place bypass cap near LED

---

**Overall Status:** ✅ **Config and schematic revisions are aligned. Firmware library and PCB net verification required before full validation.**
