# XIAO ESP32-C3 / C6 Pin Compatibility Guide

**Purpose:** Ensure ESPAtomizer firmware and PCB can work with both XIAO ESP32-C3 and XIAO ESP32-C6 modules by using a unified, safe pin mapping.

**Date:** 2025-12-07

---

## Overview

Both the **XIAO ESP32-C3** and **XIAO ESP32-C6** share the same physical footprint (21×17.8 mm, 2×7 header rows), making them pin-compatible at the header level. However, the SoC variants have subtle differences in ADC channels, strapping pins, and RF configuration. This document provides a unified pin map that works safely with both boards.

---

## Comparison Table

| Feature | XIAO ESP32-C3 | XIAO ESP32-C6 | Notes |
|---------|--------------|--------------|-------|
| **Processor** | ESP32-C3 RISC-V single-core @ 160 MHz | ESP32-C6 RISC-V dual-core @ 160 MHz (HP) + 20 MHz (LP) | C6 is more powerful |
| **ADC Channels (total)** | 4 ADC channels | 7 ADC channels | C6 has more ADC inputs |
| **ADC1 (reliable)** | GPIO0–9 | GPIO0–9 | **Common: GPIO0–9 always safe** |
| **ADC2 (WiFi-shared)** | GPIO2–5, 12–20 | GPIO2–5, 12–20 | Less reliable when WiFi active |
| **Strapping Pins** | GPIO2, GPIO8, GPIO9 | GPIO2, GPIO8, GPIO9 | Avoid for critical I/O during boot |
| **RF Switch** | Single antenna (built-in) | GPIO3 (enable), GPIO14 (switch) | C6 allows external antenna control |
| **Wireless** | WiFi 802.11b/g/n, BLE 5 | WiFi 6, BLE 5, Zigbee, Thread | C6 is more capable |
| **Battery Voltage Path** | Onboard resistor divider to A0 | Onboard resistor divider to A0 | Both support 1:2 divider on A0 (GPIO4) |
| **Form Factor** | 2×7 header, 21×17.8 mm | 2×7 header, 21×17.8 mm | **Identical pin layout** |

---

## Safe GPIO Allocation for Both C3 and C6

### Left-Row Module Pads (PCB Footprint)

| Pad | XIAO Label | GPIO | ADC (C3) | ADC (C6) | ESPAtomizer Use | Status |
|-----|-----------|------|----------|----------|-----------------|--------|
| 1 | D0 | 0 | ADC1_CH0 | ADC1_CH0 | Encoder A | ⚠️ Avoid boot (now safe) |
| 2 | D1 | 1 | ADC1_CH1 | ADC1_CH1 | I2C SCL (OLED) | ✅ Safe |
| 3 | D2 | 2 | ADC1_CH2 (strapping) | ADC1_CH2 (strapping) | I2C SDA (OLED) | ⚠️ Strapping pin |
| 4 | D3 | 21 | ❌ NO ADC | ❌ NO ADC | ~~BAT_ADC~~ (moved) | ❌ Not ADC-capable |
| 5 | D4 | 22 | ❌ NO ADC | ❌ NO ADC | Encoder SW | ✅ Safe (digital only) |
| 6 | D5 | 23 | ❌ NO ADC | ❌ NO ADC | Encoder B | ✅ Safe (digital only) |
| 7 | D6 | 16 | ❌ NO ADC | ❌ NO ADC | MOSFET_GATE | ✅ Safe (not boot, not ADC) |

### Right-Row Module Pads (PCB Footprint)

| Pad | XIAO Label | GPIO | ADC (C3) | ADC (C6) | ESPAtomizer Use | Status |
|-----|-----------|------|----------|----------|-----------------|--------|
| 8 | D7 | 17 | ❌ NO ADC | ADC2_CH6 | Spare | ✅ Available |
| 9 | D8 | 19 | ADC2_CH2 (WiFi-shared) | ADC2_CH7 (WiFi-shared) | MAX6675_SO | ⚠️ WiFi/BLE shared |
| 10 | D9 | 20 | ADC2_CH3 (WiFi-shared) | ADC2_CH8 (WiFi-shared) | MAX6675_CS | ⚠️ WiFi/BLE shared |
| 11 | D10 | 18 | ADC2_CH1 (WiFi-shared) | ADC2_CH9 (WiFi-shared) | MAX6675_SCK | ⚠️ WiFi/BLE shared |
| 12 | 5V | — | — | — | Power rail | ✅ Safe |
| 13 | GND | — | — | — | Power rail | ✅ Safe |
| 14 | 3V3 | — | — | — | Power rail | ✅ Safe |

---

## ADC Pin Selection for BAT_ADC

### Problem
- Original firmware used GPIO21 for `BAT_ADC`, which is **NOT ADC-capable** on either C3 or C6.
- Battery voltage measurement was failing.

### Solution
Move `BAT_ADC` to an ADC1-capable pin that is **safe on both C3 and C6**.

**Candidates:**
- **GPIO3** → ADC1_CH3 (C3: ✓ available, C6: ✓ available) — **RECOMMENDED**
  - A0 on the right-row header (physically near the USB)
  - Not a strapping pin
  - Reliable ADC1 input on both boards
  
- **GPIO4** → ADC1_CH4 (C3: ✓ available, C6: ✓ available) — **ALTERNATIVE**
  - Also safe on both boards
  
- **GPIO5** → ADC1_CH5 (C3: ✓ available, C6: ✓ available) — **CURRENT CHOICE**
  - Also safe on both boards

### Recommendation
**Use GPIO4 (A0 pad, default XIAO battery monitoring pin)** as the primary choice for maximum compatibility. However, **GPIO5 (currently selected in firmware) also works**.

**Why GPIO4 (A0)?**
- It's the **default onboard battery divider pin** on both C3 and C6 (per official Seeed docs).
- The modules have a built-in 1:2 resistor divider from the battery to GPIO4.
- This is the most "official" and reliable choice.

**Current firmware uses GPIO5**, which is also safe. If you want maximum compatibility with existing XIAO examples, switch to GPIO4.

---

## C3-Only Considerations (Strapping Pins)

**GPIO2, GPIO8, GPIO9** are strapping pins on the **ESP32-C3**. Their state during boot determines the chip mode:

| GPIO | C3 Strapping | C6 Strapping | ESPAtomizer Use | Risk |
|------|-------------|------------|-----------------|------|
| GPIO2 | ⚠️ Strapping | ⚠️ Strapping | I2C_SDA (OLED) | Low (pulled high by I2C) |
| GPIO8 | ⚠️ Strapping | ⚠️ Strapping | Unused | None |
| GPIO9 | ⚠️ Strapping | ⚠️ Strapping | Unused | None |

**Mitigation:** Both C3 and C6 have **internal pull-up/down resistors configured by default**. I2C_SDA (GPIO2) is pulled high by design; no additional action needed.

---

## C6-Only Features (RF Switch)

The **XIAO ESP32-C6** has an RF antenna switch controlled by GPIO3 and GPIO14:

```cpp
// To switch to external antenna (C6 only):
pinMode(3, OUTPUT);       // GPIO3 = RF switch enable
digitalWrite(3, LOW);     // Activate RF switch control

pinMode(14, OUTPUT);      // GPIO14 = antenna select
digitalWrite(14, HIGH);   // Switch to external antenna
```

**ESPAtomizer Impact:** Currently, firmware does not use GPIO3 or GPIO14. If using the XIAO ESP32-C6 and an external antenna is needed, add the above code to `setup()`.

---

## Unified Pin Mapping (Both C3 and C6)

### Confirmed Safe Mapping

```cpp
// Unified GPIO assignments for XIAO ESP32-C3 and ESP32-C6

// Power & ground (always safe)
// 3V3, GND, VBUS (5V)

// Digital I/O (no ADC required)
#define MOSFET_GATE_PIN   16   // D6: PWM output to heater MOSFET (safe on both)
#define ENC_PIN_A         0    // D0: Encoder A (was boot conflict, now safe after MOSFET moved)
#define ENC_PIN_B         23   // D5: Encoder B (safe on both)
#define ENC_PIN_SW        22   // D4: Encoder switch (safe on both)

// I2C (digital, no ADC)
#define OLED_SDA          2    // D1: I2C data (strapping pin but safe for I2C; pulled high)
#define OLED_SCL          1    // D2: I2C clock (safe on both)

// SPI (digital, no ADC)
#define MAX6675_SCK       18   // D10: SPI clock (WiFi/BLE shared, acceptable if BLE disabled)
#define MAX6675_CS        20   // D9:  SPI chip-select (WiFi/BLE shared)
#define MAX6675_SO        19   // D8:  SPI data-in (WiFi/BLE shared)

// ADC (safe on both C3 and C6)
#define BAT_ADC_PIN       5    // ADC1_CH5: Battery voltage (moved from GPIO21)
// Alternative: Use GPIO4 (A0) if you prefer the default XIAO battery pin
// #define BAT_ADC_PIN    4    // ADC1_CH4: Default XIAO A0 battery divider (recommended)

// Spare
#define SPARE_GPIO        17   // D7: Available for future use
```

---

## Hardware Update Checklist

### For XIAO ESP32-C3 & C6 Interchangeability

- [ ] **Firmware:** `config.h` updated to use GPIO5 (or GPIO4) for `BAT_PIN` ✓ (done)
- [ ] **Firmware:** Output pin confirmed on GPIO16 (MOSFET_GATE) ✓ (done)
- [ ] **PCB Schematic:** Update `BAT_ADC` net to route to GPIO5 pad on XIAO (or GPIO4 if preferred)
- [ ] **PCB Schematic:** Verify all other nets (`MOSFET_GATE`, `ENC_A/B/SW`, `MAX_SCK/CS/SO`, `SDA/SCL`) match the assignments above
- [ ] **PCB Gerber:** Regenerate Gerber files for manufacturing
- [ ] **Testing:** Test with both a XIAO ESP32-C3 and XIAO ESP32-C6 board before production
  - [ ] Battery ADC reads non-zero values
  - [ ] Encoder responds to rotation/press
  - [ ] Thermocouple (MAX6675) reads temperature
  - [ ] OLED displays correctly
  - [ ] Heater MOSFET turns on/off via PWM

---

## Board Selection in Arduino IDE

### XIAO ESP32-C3
```
Board: XIAO_ESP32C3
Board Manager: esp32 (version 3.0.0+)
Upload Speed: 921600 baud
Flash Mode: DIO
```

### XIAO ESP32-C6
```
Board: XIAO_ESP32C6
Board Manager: esp32 (version 3.0.0+)
Upload Speed: 921600 baud
Flash Mode: DIO
```

**Note:** Same firmware compiles and runs on both boards without modification (given the unified pin map above).

---

## Future Expansion (AD8495 Thermocouple Amplifier)

If you add an analog thermocouple amplifier (e.g., AD8495):

**Route the AD8495 analog output to GPIO3 or GPIO4:**
- GPIO3 → ADC1_CH3 (safe on both C3 and C6)
- GPIO4 → ADC1_CH4 (safe on both C3 and C6) — **currently used for BAT_ADC if switched**

**Do NOT use GPIO2, 8, or 9** for analog inputs on C3 (strapping pins), though they are ADC-capable.

---

## Testing Commands (Arduino IDE Serial Monitor)

Once flashed to either board, run this sketch snippet to verify ADC and pin functionality:

```cpp
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Print ADC readings
  Serial.println("=== ADC Test ===");
  Serial.print("GPIO5 (BAT_ADC): ");
  Serial.println(analogRead(5));
  
  Serial.print("GPIO4 (A0): ");
  Serial.println(analogRead(4));
  
  Serial.println("=== Digital Pin Test ===");
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  Serial.println("GPIO16 (MOSFET_GATE) set HIGH");
  delay(500);
  digitalWrite(16, LOW);
  Serial.println("GPIO16 (MOSFET_GATE) set LOW");
}

void loop() {}
```

---

## Conclusion

Both **XIAO ESP32-C3** and **XIAO ESP32-C6** can be used interchangeably with ESPAtomizer using the unified pin mapping documented above. The key differences (additional ADC channels on C6, RF switch on C6) do not conflict with the current firmware design. Simply swap the board, select the correct variant in Arduino IDE, and upload the same firmware binary.

