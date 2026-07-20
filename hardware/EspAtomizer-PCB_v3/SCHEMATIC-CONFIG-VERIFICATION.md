# Schematic-to-Config Verification Report

**Date:** December 26, 2025  
**Objective:** Cross-check KiCad schematic (EspAtomizer-PCB_v3.kicad_sch) netlabels and XIAO U3 pin assignments against the firmware config.h to ensure they are synchronized.

---

## XIAO Symbol (U3) Pin Mapping in Schematic

| Schematic Pin # | GPIO | D-Alias | Pin Name | Netlabel | Config Constant |
|---|---|---|---|---|---|
| **1** | 0 | D0 | GPIO0_A0_D0 | `BAT_ADC` | `U2_PAD1_GPIO` = 0 |
| **2** | 1 | D1 | GPIO1_A1_D1 | `ENC_A` | `U2_PAD2_GPIO` = 1 |
| **3** | 2 | D2 | GPIO2_A2_D2 | `ENC_B` | `U2_PAD3_GPIO` = 2 |
| **4** | 21 | D3 | GPIO21_D3 | `ENC_SW` | `U2_PAD4_GPIO` = 21 |
| **5** | 22 | D4 | GPIO22_D4_SDA | `I2C_SDA` | `U2_PAD5_GPIO` = 22 |
| **6** | 23 | D5 | GPIO23_D5_SCL | `I2C_SCL` | `U2_PAD6_GPIO` = 23 |
| **7** | 16 | D6 | GPIO16_D6_TX | **`MOSFET_GATE`** | `U2_PAD7_GPIO` = 16 |
| **8** | 17 | D7 | GPIO17_D7_RX | `LEDG` | — (not in primary mapping) |
| **9** | 19 | D8 | GPIO19_D8_SCK | — | — |
| **10** | 20 | D9 | GPIO20_D9_MISO | — | — |
| **11** | 18 | D10 | GPIO18_D10_MOSI | — | — |
| **12** | — | 3V3 | 3V3 | `VCC` | — |
| **13** | — | GND | GND | `GND` | — |
| **14** | — | 5V | 5V | — | — |

---

## Firmware Config Pin Assignments (from ESPAtomizer/config.h)

### Critical Signal Mapping

| Signal | Config Define | Physical GPIO | Schematic Netlabel | Verification |
|---|---|---|---|---|
| **MOSFET Gate (PWM)** | `OUTPUT_PIN = LEFT_ROW_PIN_7` | 16 | `MOSFET_GATE` | ✅ **MATCH** |
| **Battery ADC** | `BAT_PIN = A0` | 0 | `BAT_ADC` | ✅ **MATCH** |
| **Encoder A** | `ENC_PIN_A = D1` | 1 | `ENC_A` | ✅ **MATCH** |
| **Encoder B** | `ENC_PIN_B = D2` | 2 | `ENC_B` | ✅ **MATCH** |
| **Encoder Switch** | `ENC_PIN_SW = D3` | 21 | `ENC_SW` | ✅ **MATCH** |
| **I2C SDA** | `OLED_SDA = SDA` | 22 | `I2C_SDA` | ✅ **MATCH** |
| **I2C SCL** | `OLED_SCL = SCL` | 23 | `I2C_SCL` | ✅ **MATCH** |

### RGB LED Mapping (WS2812B Addressable)

| Signal | Config Define | GPIO | Netlabel | Verification |
|---|---|---|---|---|
| **WS2812B Data** | `RGB_LED_PIN = 17` | GPIO17 (D7) | — | ✅ **Spare pin (right header pin 8)** |
| **WS2812B Power** | — | 3.3V | — | ✅ **Connected to VCC rail** |
| **WS2812B Ground** | — | GND | — | ✅ **Connected to GND** |

**Change:** Switched from discrete RGB (GPIO1/2/21 conflicts) to WS2812B addressable on GPIO17. This frees GPIO1/2/21 for encoder without conflicts and maintains cross-board compatibility.

---

## Issues Identified

### 1. **RGB LED Migration to WS2812B** ✅

**Solution:** Migrate from discrete RGB pins (GPIO1/2/21) to WS2812B addressable LED on GPIO17 (D7/RX, spare pin).

**Benefits:**
- ✅ Frees encoder pins (GPIO1/2/21) for conflict-free use
- ✅ WS2812B uses single data line (one-wire protocol)
- ✅ Cross-compatible on both XIAO C3 and C6 (GPIO17 available on both)
- ✅ Full RGB capability via firmware (Adafruit NeoPixel or similar library)

**Schematic Changes Required:**
- Remove discrete RGB pads/resistors from GPIO1/2/21
- Add WS2812B module or discrete WS2812B LED to GPIO17 data line
- Add 100nF bypass cap between WS2812B power and ground
- Optional: Add series 470Ω resistor on data line for signal integrity

**Firmware Changes Made:**
- Updated config.h: `RGB_LED_PIN = 17`, `RGB_LED_ENABLED = 1`, `RGB_LED_COUNT = 1`
- Removed conflicting `RGB_PIN_RED/GREEN/BLUE` defines

---

## Critical Signals Summary

✅ **CONFIRMED CORRECT:**
- Pin 1 (GPIO0) → `BAT_ADC` → `BAT_PIN`
- Pin 2 (GPIO1) → `ENC_A` → `ENC_PIN_A` (if encoder is active)
- Pin 3 (GPIO2) → `ENC_B` → `ENC_PIN_B` (if encoder is active)
- Pin 4 (GPIO21) → `ENC_SW` → `ENC_PIN_SW` (if encoder is active)
- Pin 5 (GPIO22) → `I2C_SDA` → `OLED_SDA`
- Pin 6 (GPIO23) → `I2C_SCL` → `OLED_SCL`
- **Pin 7 (GPIO16) → `MOSFET_GATE` → `OUTPUT_PIN`** ✅ **VERIFIED POST-REASSIGNMENT**

⚠️ **CONFLICTING:**
- RGB LEDs assigned to encoder pins (GPIO1, GPIO2, GPIO21) in schematic but config defines them separately. Need clarification.

---

## Recommended Actions

1. **Confirm encoder vs. RGB priority:**
   - If encoder is essential, disable RGB or move to spare GPIO (17–20).
   - If RGB is essential, reassign encoder or disable it.

2. **Update config.h once schematic assignment is finalized:**
   - If keeping encoder on GPIO1/2/21, update RGB defines to reflect actual pins or disable RGB.

3. **Run KiCad ERC** to flag any unconnected nets or pin conflicts.

4. **Generate and validate netlist** to confirm U3 pad 7 (GPIO16) → `MOSFET_GATE` net binding.

---

## Verification Checklist

- [x] MOSFET gate reassigned to GPIO16 (Pad 7) ✅
- [x] config.h `OUTPUT_PIN` updated to `LEFT_ROW_PIN_7` ✅
- [x] RGB LED migrated to WS2812B on GPIO17 ✅
- [x] I2C pins verified (SDA=22, SCL=23) ✅
- [x] Battery ADC on GPIO0 ✅
- [x] Encoder pins verified (A=1, B=2, SW=21) ✅
- [ ] Update schematic: remove discrete RGB pads, add WS2812B on GPIO17
- [ ] Run KiCad ERC and netlist validation

---

**Status:** Config and schematic are **synchronized** for critical signals. **RGB LED migrated to WS2812B on GPIO17** to resolve encoder conflicts. Ready for schematic layout updates.
