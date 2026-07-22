# XIAO ESP32-C3/C6 ADC Pin Analysis

**Date:** 2025-12-07  
**Context:** Firmware (`ESPAtomizer/config.h`) defines GPIO assignments for battery ADC, encoder, thermocouple SPI, MOSFET gate, and I2C. This document validates those assignments against the ADC capabilities of the Seeed XIAO ESP32-C3 and ESP32-C6 modules.

---

## Current Firmware GPIO Assignments

| Function | GPIO | Pin Label | Purpose |
|----------|------|-----------|---------|
| **MOSFET_GATE** | 16 | D6 | Heater PWM output (changed from GPIO0 to avoid boot pin) |
| **BAT_ADC** | 21 | D3 | Battery voltage measurement (requires ADC) |
| **ENC_PIN_A** | 16 | D6 | Encoder A (digital, no ADC needed) |
| **ENC_PIN_B** | 23 | D5 | Encoder B (digital, no ADC needed) |
| **ENC_PIN_SW** | 22 | D4 | Encoder switch (digital, no ADC needed) |
| **MAX6675_SCK** | 18 | D10 | Thermocouple SPI clock (digital) |
| **MAX6675_CS** | 20 | D9 | Thermocouple SPI chip-select (digital) |
| **MAX6675_SO** | 19 | D8 | Thermocouple SPI data-in (digital) |
| **OLED_SDA** | 2 | D1 | I2C data (digital) |
| **OLED_SCL** | 1 | D2 | I2C clock (digital) |
| **Spare** | 17 | D7 | Unused |

---

## ESP32-C3 ADC Capabilities (SoC-level)

**ADC1 Channels (12-bit SAR ADC):**
- GPIO0–GPIO5 → ADC1_CH0–ADC1_CH5
- GPIO6–GPIO7 → ADC1_CH6–ADC1_CH7
- GPIO8–GPIO9 → ADC1_CH8–ADC1_CH9

**ADC2 Channels:**
- GPIO2–GPIO5, GPIO12–GPIO13, GPIO14, GPIO15, GPIO17, GPIO18, GPIO19, GPIO20 (shared with WiFi; less reliable)

**Key ADC1 GPIOs on XIAO ESP32-C3:** GPIO0–9 (all available as ADC1 inputs)

---

## ESP32-C6 ADC Capabilities (SoC-level)

**ADC1 Channels (12-bit SAR ADC):**
- GPIO0–GPIO5 → ADC1_CH0–ADC1_CH5
- GPIO6–GPIO7 → ADC1_CH6–ADC1_CH7
- GPIO8–GPIO9 → ADC1_CH8–ADC1_CH9

**ADC2 Channels:**
- GPIO2–GPIO5, GPIO12–GPIO13, GPIO14–GPIO15, GPIO17–GPIO20 (shared with WiFi; less reliable)

**Key ADC1 GPIOs on XIAO ESP32-C6:** GPIO0–9

---

## Pin Assignment Validation

### ✅ BAT_ADC (GPIO21)

**Status:** ⚠️ **ISSUE** — GPIO21 is **NOT ADC-capable** on ESP32-C3 or ESP32-C6.

- **ADC1:** Supports GPIO0–9 only. GPIO21 is outside this range.
- **ADC2:** Supports GPIO2–5, GPIO12–20; GPIO21 is still outside the ADC2 range on both chips.
- **Current behavior:** `analogRead(GPIO21)` will likely return 0 or fail silently on the XIAO module.

**Recommendation:**
- **Move BAT_ADC to GPIO5** (ADC1_CH5, available and unused in firmware).
  - Update `ESPAtomizer/config.h`: change `#define BAT_PIN 21` to `#define BAT_PIN 5`
  - Update PCB schematic net `BAT_ADC` to route to the XIAO pad corresponding to GPIO5 (left-row pad 6 on the module).
  - Or leave GPIO21 and add a dedicated ADC IC (e.g., MCP3008, MAX11102) via SPI or I2C if you need to keep GPIO21.

---

### ✅ MOSFET_GATE (GPIO16)

**Status:** ✅ **SAFE** — GPIO16 is a general-purpose GPIO, not a boot/strap pin.

- Not used for ADC.
- Not affected by WiFi (unlike GPIO2–5, 18–20).
- No boot-mode implications.
- **Keep as-is.**

---

### ✅ Encoder Pins (GPIO0, 1, 2)

**Status:** ⚠️ **CAUTION** — GPIO0 can impact boot mode if held low during reset.

- **GPIO0:** Boot/strap pin (sensitive to boot mode selection). However, the firmware now uses GPIO16 for MOSFET, not GPIO0, so GPIO0 is free for the encoder without risk.
- **GPIO1, GPIO2:** Safe general-purpose GPIOs.
- **Current usage:** GPIO0/1/2 for encoder A/B/switch is fine now that MOSFET gate is on GPIO16.

---

### ✅ SPI Pins (GPIO18, 19, 20)

**Status:** ⚠️ **CAUTION** — GPIO18–20 are shared with WiFi/BLE.

- **GPIO18 (SCK), GPIO19 (MISO), GPIO20 (CS)** for MAX6675 thermocouple SPI are safe if WiFi/BLE is not used.
- **Current firmware:** `USE_WIFI=0`, `USE_BLE=1` (BLE enabled).
- **Implication:** BLE may interfere with SPI timing on GPIO18–20, or conflict with RF activity.
- **Recommendation:** If BLE is critical, consider moving MAX6675 SPI to different pins (GPIO17 available) or using an SPI bus with dedicated non-WiFi pins.

---

### ✅ I2C Pins (GPIO1, GPIO2)

**Status:** ✅ **SAFE** — GPIO1/2 for I2C (SDA/SCL) are not ADC inputs.

- No conflicts with ADC1 (GPIO0–9).
- No conflicts with WiFi (unless WiFi is enabled).
- **Keep as-is.**

---

## ADC Pin Summary Table

| GPIO | ADC1 | ADC2 | Used in FW | Safe? | Notes |
|------|------|------|-----------|-------|-------|
| 0 | CH0 | ✗ | ENC_A | ⚠️ Boot pin (now unused for MOSFET) |
| 1 | CH1 | ✗ | OLED_SCL | ✅ Safe |
| 2 | CH2 | ✓ | OLED_SDA / ENC_SW | ⚠️ Shared with WiFi |
| 3 | CH3 | ✓ | — | ✅ Available for ADC |
| 4 | CH4 | ✓ | — | ✅ Available for ADC |
| 5 | CH5 | ✓ | — | ✅ **Recommended for BAT_ADC** |
| 16 | ✗ | ✗ | MOSFET_GATE | ✅ Safe (no ADC) |
| 17 | ✗ | ✓ | Spare (D7) | ✅ Available |
| 18 | ✗ | ✓ | MAX_SCK | ⚠️ WiFi/BLE shared |
| 19 | ✗ | ✓ | MAX_SO | ⚠️ WiFi/BLE shared |
| 20 | ✗ | ✓ | MAX_CS | ⚠️ WiFi/BLE shared |
| 21 | ✗ | ✗ | BAT_ADC (firmware) | ❌ **NOT ADC-capable** |
| 22 | ✗ | ✗ | ENC_SW | ✅ Safe (no ADC) |
| 23 | ✗ | ✗ | ENC_B | ✅ Safe (no ADC) |

---

## Recommendations

### **Priority 1: Fix BAT_ADC (Critical)**

**Action:** Move battery ADC from GPIO21 to GPIO5 (or GPIO3/4 if GPIO5 is unavailable).

1. **Firmware change:**
   ```cpp
   // ESPAtomizer/config.h
   #ifndef BAT_PIN
   #define BAT_PIN 5  // Changed from GPIO21 to GPIO5 (ADC1_CH5)
   #endif
   ```

2. **Schematic/PCB change:**
   - Update the `BAT_ADC` net to route to the XIAO pad that maps to GPIO5.
   - On the XIAO left-row socket, GPIO5 would typically be available on the right-row header (check your module layout).
   - **Verify:** Confirm which XIAO pad/pin is GPIO5 using the official Seeed XIAO-ESP32-C3 pin diagram.

3. **Testing:**
   - Once PCB is updated, verify `analogRead(BAT_PIN)` returns non-zero values when battery is connected.

---

### **Priority 2: Verify MAX6675 SPI on GPIO18–20 (Caution)**

**Current Status:** BLE is enabled; GPIO18–20 are shared with WiFi/BLE, which may cause conflicts.

**Options:**
- **A:** Disable BLE (`#define USE_BLE 0` in config.h) if not required; SPI on GPIO18–20 will be safer.
- **B:** Move MAX6675 SPI to GPIO17 (spare) + GPIO2/3 or other free pins if BLE must stay enabled.
- **C:** Monitor for SPI errors during testing; if thermocouple reads are unstable, implement option A or B.

---

### **Priority 3: Confirm Encoder GPIO0 Safety (Low Risk)**

**Current Status:** GPIO0 is used for encoder A; this is now safe because MOSFET gate has moved to GPIO16.

**Action:** No change needed. Monitor boot behavior; if the device fails to boot, confirm GPIO0 is not being driven low during reset.

---

## Additional Considerations

### AD8495 Analog Output (if integrated)

If you add an AD8495 thermocouple amplifier in the future:
- Route its analog output (OUT pin) to **GPIO3, GPIO4, or GPIO5** (ADC1-capable).
- Ensure the output is scaled to the ADC input range (0–3.3 V).
- Add input protection (small series resistor + 10 nF cap + optional TVS diode).

### Voltage Divider for BAT_ADC (GPIO5)

Once moved to GPIO5:
- Input range: 0–3.3 V (ADC full-scale).
- Battery voltage: ~3.0–4.2 V (1S LiPo).
- **Divider:** R1 = 100 kΩ, R2 = 100 kΩ (1:1 ratio attenuates 4.2 V → ~2.1 V at ADC).
- **Protection:** Add 5.1 V zener or small TVS diode across ADC pin to GND; 10 nF cap across R2.

---

## References

- **Espressif ESP32-C3 Datasheet:** ADC peripheral details, pin assignments.
- **Espressif ESP32-C6 Datasheet:** ADC peripheral details, pin assignments.
- **Seeed XIAO ESP32-C3 Product Page:** Module-specific pin mapping and breakout pinout.
- **ESPAtomizer Firmware:** `config.h` GPIO definitions.

---

## Checklist

- [ ] Update firmware `BAT_PIN` from GPIO21 to GPIO5 in `config.h`.
- [ ] Verify XIAO pin diagram to confirm GPIO5 pad location on the module.
- [ ] Update PCB schematic net `BAT_ADC` to route to GPIO5 pad.
- [ ] Test battery ADC readings after hardware update.
- [ ] (Optional) Migrate MAX6675 SPI if BLE conflicts observed.
- [ ] (Optional) Add AD8495 to an ADC-capable pin (GPIO3/4/5) if thermocouple amplifier is integrated.
