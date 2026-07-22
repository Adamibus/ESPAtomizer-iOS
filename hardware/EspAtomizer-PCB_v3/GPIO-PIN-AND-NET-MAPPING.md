# GPIO Pin & Net Label Mapping for ESPAtomizer v3

Complete reference for PCB schematic net labels and corresponding ESP32-C6 GPIO assignments with ADS1115 I2C thermocouple converter.

---

## **Quick Reference: GPIO → Net → Function**

| GPIO | ADC | Net Label | Function | Component/Connector | Firmware Macro |
|------|-----|-----------|----------|-------------------|----------------|
| **0** | ADC1_CH0 | BAT_ADC | Battery voltage measurement | Battery divider | `BAT_PIN` |
| **1** | ADC1_CH1 | – | Available | – | – |
| **2** | ADC1_CH2 | _Unused_ | (Thermocouple now via I2C) | – | – |
| **16** | – | MOSFET_GATE | Heater MOSFET gate (PWM) | Q1 (PSMN1R0-30YLDX) | `OUTPUT_PIN` |
| **17** | – | ENC_A | Encoder channel A | ENC1 (rotary encoder) | `ENC_PIN_A` |
| **18** | – | – | Available (SPI_MOSI) | – | – |
| **19** | – | I2C_SCL | I2C Clock (OLED + ADS1115) | J4 (OLED) + U1 (thermocouple) | `OLED_SCL` |
| **20** | – | I2C_SDA | I2C Data (OLED + ADS1115) | J4 (OLED) + U1 (thermocouple) | `OLED_SDA` |
| **21** | – | – | Not used (non-ADC) | – | – |
| **22** | – | ENC_SW | Encoder switch (button) | ENC1 (rotary encoder) | `ENC_PIN_SW` |
| **23** | – | ENC_B | Encoder channel B | ENC1 (rotary encoder) | `ENC_PIN_B` |

---

## **Power & Ground Nets**

| Net Label | Source | Voltage | Connected To | Notes |
|-----------|--------|---------|--------------|-------|
| **VCC** | 3.3V regulator | 3.3V | U1 (ADS1115), XIAO module, OLED, peripherals | Logic supply only (3.3V design) |
| **GND** | Ground plane | 0V | All ground pins | Common return |

---

## **Analog Nets (ADC Inputs)**

| Net Label | GPIO | ADC Channel | Source | Purpose | Value Range | Firmware |
|-----------|------|-------------|--------|---------|-------------|----------|
| **BAT_ADC** | 0 | ADC1_CH0 | Battery divider | Battery voltage monitor | 0–3.3V (3.0–4.2V scaled) | `analogRead(BAT_PIN)` |

---

## **I2C Thermocouple Nets (ADS1115)**

| Net Label | GPIO | Function | Connector | Destination | I2C Address | Notes |
|-----------|------|----------|-----------|-------------|-------------|-------|
| **I2C_SDA** | 20 | I2C Data | U1 pin 4 (ADS1115) + J4 (OLED) | Multi-master I2C bus | 0x48 (ADS1115) / 0x3C (OLED) | Shared with OLED |
| **I2C_SCL** | 19 | I2C Clock | U1 pin 3 (ADS1115) + J4 (OLED) | Multi-master I2C bus | 0x48 (ADS1115) / 0x3C (OLED) | Shared with OLED |

**ADS1115 Thermocouple Input Pins:**
- U1 pin 5 (AIN0) ← Thermocouple input (K-type, positive)
- U1 pin 6 (AIN1) ← Thermocouple input (K-type, negative/reference, differential)

---

## **Heater Output Net**

| Net Label | Source | Destination | Purpose | Notes |
|-----------|--------|-------------|---------|-------|
| **MOSFET_GATE** | GPIO16 (OUTPUT_PIN) | Q1 gate | Heater on/off control | PWM @ 200 Hz, 10-bit resolution |

---

## **I2C Nets (OLED Display)**

| Net Label | GPIO | Function | Connector | Destination | Notes |
|-----------|------|----------|-----------|-------------|-------|
| **I2C_SDA** | 20 | I2C Data | J4 (1×4 header) | OLED module SDA | Address 0x3C, shared with ADS1115 |
| **I2C_SCL** | 19 | I2C Clock | J4 (1×4 header) | OLED module SCL | ~400 kHz, shared with ADS1115 |

---

## **Encoder Nets**

| Net Label | GPIO | Function | Connector | Destination | Notes |
|-----------|------|----------|-----------|-------------|-------|
| **ENC_A** | 17 | Channel A | ENC1 | Rotary encoder A | 2 edges/detent |
| **ENC_B** | 23 | Channel B | ENC1 | Rotary encoder B | Orthogonal to A |
| **ENC_SW** | 22 | Switch (button) | ENC1 | Rotary encoder SW | Active low, debounce 50ms |

---

## **Connector Pinout Reference**

### **U1: ADS1115 Thermocouple IC (SOIC-8)**
```
[1]—ADDR———→ GND (sets I2C address to 0x48)
[2]—GND———→ Ground plane
[3]—SCL———→ GPIO19 (I2C clock, shared with OLED)
[4]—SDA———→ GPIO20 (I2C data, shared with OLED)
[5]—AIN0———→ Thermocouple input (K-type positive)
[6]—AIN1———→ Thermocouple input (K-type negative/reference)
[7]—ALRT———→ Not used (floating or GND)
[8]—VDD———→ 3.3V supply + 100nF bypass cap to GND
```

### **J1: Thermocouple Input (1×2 Header)**
```
[1]—THK+———→ ADS1115 pin 5 (AIN0)
[2]—THK−———→ ADS1115 pin 6 (AIN1)
```
**Typical wiring:** K-type thermocouple (red = +, yellow = −)

### **J4: OLED Display (1×4 Header)**
```
[1]—VCC———→ OLED VDD
[2]—GND———→ OLED GND
[3]—I2C_SDA———→ OLED SDA (GPIO 20, I2C address 0x3C)
[4]—I2C_SCL———→ OLED SCL (GPIO 19)
```

### **J5: Heater Output (1×2 Header)**
```
[1]—+5V or Battery+
[2]—MOSFET Drain (Q1 pin 2) → Heater Load
```

---

## **XIAO Module Left-Row Pin Mapping**

XIAO ESP32-C6 left-row header pads:

| Pad | GPIO | Label | Current Use |
|-----|------|-------|------------|
| Pad 1 | GPIO 0 | D0 | BAT_ADC (battery voltage, ADC1_CH0) |
| Pad 2 | GPIO 1 | D1 | Available |
| Pad 3 | GPIO 2 | D2 | _Unused_ (thermocouple now I2C) |
| Pad 4 | GPIO 21 | D3 | ~~Not used~~ (non-ADC on C6) |
| Pad 5 | GPIO 22 | D4 | ENC_SW (encoder switch) |
| Pad 6 | GPIO 23 | D5 | ENC_B (encoder B) |
| Pad 7 | GPIO 16 | D6 | MOSFET_GATE (heater PWM) |

**Right-row header pads:**

| Pad | GPIO | Label | Current Use |
|-----|------|-------|------------|
| Pad 8 | GPIO 17 | D7 | ENC_A (encoder channel A) |
| Pad 9 | GPIO 19 | D8 | I2C_SCL (I2C clock, shared OLED + ADS1115) |
| Pad 10 | GPIO 20 | D9 | I2C_SDA (I2C data, shared OLED + ADS1115) |
| Pad 11 | GPIO 18 | D10 | Available |
| Pad 12 | — | 3V3 | Power supply |
| Pad 13 | — | GND | Ground |
| Pad 14 | — | 5V | USB power (not used in design, battery only) |

**All pins use standard 2×7 header — no castellated holes required!** ✅

---

## **Firmware Config.h Macros Summary**

```cpp
// Battery monitoring (ADC)
#define BAT_PIN 0            // GPIO0 (D0, ADC1_CH0) left-side header

// Output (heater, PWM)
#define OUTPUT_PIN 16        // GPIO16 (D6) left-side header

// Encoder pins
#define ENC_PIN_A 17         // GPIO17 (D7) right-side header
#define ENC_PIN_B 23         // GPIO23 (D5) left-side header
#define ENC_PIN_SW 22        // GPIO22 (D4) left-side header

// I2C Bus (shared OLED + ADS1115 thermocouple)
#define OLED_SDA 20          // GPIO20 (D9) right-side header
#define OLED_SCL 19          // GPIO19 (D8) right-side header

// ADS1115 Thermocouple (I2C)
#define USE_ADS1115 1        // Enable ADS1115 I2C thermocouple converter
#define ADS1115_I2C_ADDR 0x48 // I2C address (ADDR pin tied to GND)
#define ADS1115_CHANNEL 0     // ADC channel 0 for thermocouple input
#define ADS1115_SAMPLES 16    // Averaging samples for noise reduction
#define ADS1115_GAIN GAIN_ONE // ±4.096V range
```

---

## **Net Routing Checklist**

- [ ] **BAT_ADC (GPIO0):** Battery divider net → GPIO0 (pin 1 left header)
- [ ] **MOSFET_GATE (GPIO16):** PWM signal → Q1 gate with 100Ω series resistor + 100kΩ pull-down
- [ ] **ENC_A/B/SW:** Encoder → GPIO17 (pin 8 right) / GPIO23 (pin 6 left) / GPIO22 (pin 5 left)
- [ ] **I2C_SDA/SCL:** J4 (OLED) + U1 (ADS1115) → GPIO20 (pin 10 right) / GPIO19 (pin 9 right)
- [ ] **ADS1115 I2C connections:**
  - [ ] U1 pin 1 (ADDR) → GND (I2C address 0x48)
  - [ ] U1 pin 2 (GND) → Ground plane
  - [ ] U1 pin 3 (SCL) → GPIO19
  - [ ] U1 pin 4 (SDA) → GPIO20
  - [ ] U1 pin 5 (AIN0) → Thermocouple input
  - [ ] U1 pin 6 (AIN1) → Thermocouple input
  - [ ] U1 pin 8 (VDD) → 3.3V + 100nF bypass cap to GND
- [ ] **GND:** Power distribution to all ICs

---

## **Testing Procedure**

1. **Verify power supply:** VCC should be 3.3V (no external 5V in design)
2. **Test ADC GPIO0:** Should read proportional to battery voltage
3. **Verify I2C:** 
   - OLED display should initialize on GPIO20 (SDA) / GPIO19 (SCL) at address 0x3C
   - ADS1115 should respond at address 0x48 on same I2C bus
4. **Test thermocouple (ADS1115):** Should read 0–3.3V equivalent via I2C (room temp ~0.5–1.0V)
5. **Verify encoder:** ENC_A (GPIO17)/ENC_B (GPIO23)/ENC_SW (GPIO22) should toggle with rotation/press
6. **PWM output:** GPIO16 should deliver PWM signal to Q1 gate
7. **I2C multi-master:** Both OLED and ADS1115 should operate simultaneously without conflicts

---

## **Common Wiring Errors**

| Error | Symptom | Fix |
|-------|---------|-----|
| ADS1115 ADDR pin not tied to GND | I2C device not found (0x48) | Tie U1 pin 1 to GND |
| I2C_SDA/SCL shared connection fails | OLED or thermocouple doesn't communicate | Verify 4.7kΩ pull-ups on I2C bus |
| Thermocouple polarity reversed | Negative temperature readings | Swap J1 pins 1 & 2 (swap AIN0 ↔ AIN1) |
| BAT_ADC not connected | Battery monitoring fails | Check GPIO0 routing from divider |
| MOSFET_GATE not routed | Heater never turns on | Check GPIO16 connects to Q1 gate |
| Encoder pins swapped | Rotation direction inverted | Adjust pin mapping in firmware |
| ADS1115 VDD cap missing | Unstable I2C communication | Add 100nF bypass cap U1 pin 8 → GND |

---

## **Advantages of ADS1115 I2C Approach**

1. **✅ 3.3V Native:** ADS1115 operates directly from 3.3V (no 4.5V+ requirement)
2. **✅ No New Pins:** Shares existing I2C bus with OLED (GPIO19/20)
3. **✅ Multi-Master I2C:** Both devices operate at different addresses (0x48 vs 0x3C)
4. **✅ Simpler Schematic:** Single IC replaces AD8495 + TLV431 reference + filter caps
5. **✅ Cost Savings:** ~$1–2 per board vs AD8495 + reference circuit
6. **✅ Better Accuracy:** 16-bit resolution vs 8-bit ADC
7. **✅ No New Firmware Pins:** Uses existing Wire library on I2C bus

---

## **Reference Documents**

- **ADS1115 Datasheet:** https://www.ti.com/lit/ds/symlink/ads1115.pdf
- **XIAO ESP32-C6 Pinout:** https://wiki.seeedstudio.com/XIAO_ESP32C6_Getting_Started/
- **PSMN1R0-30YLDX Datasheet:** https://assets.nexperia.com/documents/data-sheet/PSMN1R0-30YLD.pdf

---

**Last Updated:** December 7, 2025 (Updated for ADS1115 I2C thermocouple)  
**Status:** Complete — Ready for PCB layout with simplified schematic
