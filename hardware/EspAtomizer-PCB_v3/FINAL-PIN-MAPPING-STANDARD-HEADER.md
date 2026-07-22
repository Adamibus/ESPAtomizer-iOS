# Final Pin Mapping — XIAO ESP32-C3/C6 (2×7 Header)

**Date:** December 25, 2025 (Revised)  
**Design Goal:** Use ONLY the standard 14-pin (2×7) header on XIAO ESP32-C3/C6 (battery +/− on back)  
**No castellated holes required!** ✅

---

**Compatibility note:** For a final compatibility checklist and board-variant guidance see: [XIAO C3/C6 Compatibility](XIAO-C3-C6-COMPATIBILITY.md). For local build/upload verification tools and examples see: [tools/README.md](../../tools/README.md)


## 📌 **Complete Pin Assignment (D/SDA/SCL macros)**

### **Left-Side Header (Pins 1-7)**

| Pin # | D Pin | Net Label | Function | Notes | Firmware |
|-------|-------|-----------|----------|-------|----------|
| **1** | D0/A0 | `BAT_ADC` | Battery voltage divider | ADC input | `BAT_PIN` |
| **2** | D1 | `ENC_A` | Encoder channel A | INPUT_PULLUP | `ENC_PIN_A` |
| **3** | D2 | `ENC_B` | Encoder channel B | INPUT_PULLUP | `ENC_PIN_B` |
| **4** | D3 | `ENC_SW` | Encoder switch/button | INPUT_PULLUP | `ENC_PIN_SW` |
| **5** | D4 | `I2C_SCL` | I2C clock (OLED + ADS1115) | Use SCL macro | `OLED_SCL` |
| **6** | D5 | `I2C_SDA` | I2C data (OLED + ADS1115) | Use SDA macro | `OLED_SDA` |
| **7** | D6 | `RGB_RED` | RGB LED red (optional) | Boot UART chatter OK | `RGB_PIN_RED` |

### **Right-Side Header (Pins 8-14)**

| Pin # | D Pin | Net Label | Function | Notes | Firmware |
|-------|-------|-----------|----------|-------|----------|
| **8** | D7/RX | `RGB_GREEN` | RGB LED green (optional) | RX capable | `RGB_PIN_GREEN` |
| **9** | D8/SCK | — | — | Avoid on C3 (strap) | — |
| **10** | D9/MISO | — | — | Avoid on C3 (BOOT button) | — |
| **11** | D10/MOSI | `MOSFET_GATE` | Heater PWM output | LEDC PWM | `OUTPUT_PIN` |
| **12** | 3V3 | `VCC` | 3.3V power output | — | — |
| **13** | GND | `GND` | Ground | — | — |
| **14** | 5V | `VS+` | 5V USB input | — | — |

---

## 🎯 **Firmware Configuration**

```cpp
// config.h — Final pin assignments (standard 2×7 header only)

// ADC Input (left header)
#define BAT_PIN A0            // D0/A0 — battery monitoring

// PWM Output (right header)
#define OUTPUT_PIN D10        // D10/MOSI — heater MOSFET gate

// Encoder (left header)
#define ENC_PIN_A D1          // Pin 2 left
#define ENC_PIN_B D2          // Pin 3 left
#define ENC_PIN_SW D3         // Pin 4 left

// I2C Bus (shared OLED + ADS1115 Thermocouple)
#define OLED_SDA SDA          // Board default SDA (D5)
#define OLED_SCL SCL          // Board default SCL (D4)
#define USE_ADS1115 1        // Enable ADS1115 I2C thermocouple converter
#define ADS1115_I2C_ADDR 0x48 // ADS1115 I2C address (K-type thermocouple)

// RGB LED Status Indicator (optional)
#define RGB_LED_ENABLED 1     // Set 0 to disable if pins are tight
#define RGB_PIN_RED D6        // D6 (tolerates boot UART chatter)
#define RGB_PIN_GREEN D7      // D7/RX
// Third channel is intentionally unassigned on C3 for safety; consider WS2812.
```

---

## 🔌 **Wiring Connections**

### **U1 (ADS1115 Thermocouple Converter IC, SOIC-8) → U3 (XIAO) via I2C**
- U1 pin 1 (ADDR) → GND (sets I2C address to 0x48)
- U1 pin 2 (GND) → GND (U3 pin 13)
- U1 pin 3 (SCL) → **SCL (D4)** via `I2C_SCL` net (shared with OLED)
- U1 pin 4 (SDA) → **SDA (D5)** via `I2C_SDA` net (shared with OLED)
- U1 pin 5 (AIN0) → Thermocouple input (K-type)
- U1 pin 6 (AIN1) → Thermocouple input (K-type, differential)
- U1 pin 7 (ALRT) → Not used (floating or GND)
- U1 pin 8 (VDD) → 3.3V (U3 pin 12)
- Add 100nF bypass cap from VDD to GND near U1

### **Battery Divider → U3 (XIAO)**
- Battery voltage divider → **D0/A0** (U3 pin 1) via `BAT_ADC` net

### **Q1 (MOSFET) → U3 (XIAO)**
- Q1 gate ← **D10** via `MOSFET_GATE` net
- Add 100Ω series resistor + 100kΩ pull-down on gate

### **RGB LED (Status Indicator) → U3 (XIAO)**
- Red channel → **D6** via 220Ω resistor (OK if it flickers at boot)
- Green channel → **D7** via 220Ω resistor
- Common cathode → **GND**
- Optional: switch to single-wire addressable LED (WS2812) on one spare D pin for full RGB

### **ENC1 (Rotary Encoder) → U3 (XIAO)**
- ENC1 terminal A → **D1** via `ENC_A` net
- ENC1 terminal B → **D2** via `ENC_B` net
- ENC1 switch → **D3** via `ENC_SW` net
- Common terminal → GND

### **J4 (OLED Connector) → U3 (XIAO) via I2C**
- J4 SDA → **SDA (D5)** via `I2C_SDA` net (shared with ADS1115)
- J4 SCL → **SCL (D4)** via `I2C_SCL` net (shared with ADS1115)
- J4 VCC → 3.3V (U3 pin 12)
- J4 GND → GND (U3 pin 13)
- **Note:** OLED uses I2C address 0x3C, ADS1115 uses 0x48 → no address conflict

---

## ✅ **Key Advantages of This Mapping**

1. **✅ No castellated holes required** — all pins on standard 2×7 header
2. **✅ Single ADC input available** — GPIO0 for battery monitoring (sufficient)
3. **✅ I2C Thermocouple (ADS1115)** — 3.3V native, shares I2C bus with OLED (no new pins!)
4. **✅ Multi-master I2C** — OLED (0x3C) and ADS1115 (0x48) coexist without conflicts
5. **✅ Hardware I2C on dedicated bus** — SCL/SDA macros (D4/D5) map per board
6. **✅ PWM on safe pins** — D10 (heater), D6/D7 OK for LEDs (D6 chatters at boot)
7. **✅ Encoder on general-purpose GPIOs** — no special requirements
8. **✅ RGB LED status indicator** — Full-color status using 3 remaining GPIO pins
9. **✅ Simpler schematic** — Single ADS1115 IC replaces AD8495 + TLV431 reference
10. **✅ Compatible with C3 and C6** — uses D/SDA/SCL macros (identical header)

---

## 📋 **Schematic Update Checklist**

Before PCB layout:

### **U3 (XIAO Module) Pin Connections:**
- [ ] Pin 1 (D0/A0) → `BAT_ADC` net (from battery divider)
- [ ] Pin 2 (D1) → `ENC_A` net (from encoder A)
- [ ] Pin 3 (D2) → `ENC_B` net (from encoder B)
- [ ] Pin 4 (D3) → `ENC_SW` net (from encoder switch)
- [ ] Pin 5 (D4) → `I2C_SCL` net (to J4 SCL)
- [ ] Pin 6 (D5) → `I2C_SDA` net (to J4 SDA)
- [ ] Pin 7 (D6) → `RGB_RED` net (optional)
- [ ] Pin 8 (D7/RX) → `RGB_GREEN` net (optional)
- [ ] Pin 11 (D10/MOSI) → `MOSFET_GATE` net (to Q1 gate)
- [ ] Pin 12 (3V3) → `VCC` net (power rail)
- [ ] Pin 13 (GND) → `GND` net (ground plane)
- [ ] Pin 14 (5V) → `VS+` net (power input)

### **Add Missing Capacitors:**
- [ ] C6 (100nF) between U2 pin 2 (`REF`) and GND
- [ ] C7 (100nF) between U1 pin 6 (`OUT_ADC`) and GND

### **Verification:**
- [ ] Run Design → Electrical Rules Check (ERC)
- [ ] Verify all nets have labels matching firmware config
- [ ] Verify no floating pins on U1, U2, U3
- [ ] Check that all power pins (VS+, VCC, GND) are properly connected

---

## 🧪 **Testing Procedure**

After PCB assembly:

1. **Power test:**
   - Measure 3.3V on U3 pin 12
   - Measure 5V on U3 pin 14
   - Verify GND continuity on U3 pins 13/16

2. **Reference voltage:**
   - Measure ~1.24V on `REF` net (U2 pin 2)

3. **ADC inputs:**
   - **D0/A0 (Battery):** Should read ~1.65V for 3.7V battery (50% divider)

4. **Encoder:**
   - Rotate encoder: D1/D2 should toggle (check with oscilloscope or LED)
   - Press switch: D3 should go LOW when pressed

5. **I2C OLED:**
   - Upload firmware
   - OLED should initialize and display boot screen
   - If not working: check SDA/SCL signals with logic analyzer

6. **PWM output:**
   - D10 should output PWM signal (~200 Hz, 10-bit)
   - Measure with oscilloscope on Q1 gate

---

## 🔄 **Migration Summary**

### **Changes from Previous Version:**

| Function | Old GPIO | New GPIO | Reason |
|----------|----------|----------|--------|
| Thermocouple ADC | 3 (castellated) → 4 (castellated) | **2 (header)** | Avoid castellated holes |
| Battery ADC | 5 (castellated) | **0 (header)** | Already on header ✅ |
| Encoder A | 0 → 6 (castellated) | **17 (header)** | Free up GPIO0, avoid castellated |
| OLED SDA | 2 → 1 | **20 (header)** | Free up GPIO2 for ADC |
| OLED SCL | 1 | **19 (header)** | Move with SDA |

**Net result:** All 8 active signal pins now use standard 2×7 header!

---

## 📖 **Pin Usage Summary**

**Used Pins:** 7–9 of 11 GPIO pins
- Left header: D0/A0, D1, D2, D3, D4(SCL), D5(SDA), D6(optional)
- Right header: D7(optional), D10 (MOSFET)

**Available Pins:** Depends on RGB usage
- If using 2-channel RGB on D6/D7: free pins include D8, D9 (avoid on C3), or a D-pin freed by moving encoder

**Reserved Pins:**
- Power: 3V3, GND, 5V
- JTAG/Debug: Top header (optional, not in main 2×7)

---

## ⚙️ **GPIO Capability Verification (C3/C6)**

### **ADC Pins (Analog Input)**
- Use `A0` (D0) for battery; maps to ADC1 on both C3/C6.
- Avoid `A3` on ESP32-C3 (ADC2 reliability note from Seeed docs).

### **I2C Pins (Hardware I2C)**
- Use `SCL` (D4) and `SDA` (D5) macros for portability; Arduino core maps them per board.
- 400 kHz supported; add 4.7k pull-ups if OLED module lacks them.

### **PWM Pin (LEDC Output)**
- Use `D10` for MOSFET PWM; safe on C3 (not strap/BOOT), supports LEDC on C3/C6.

### **Digital Input Pins (Encoder)**
- `D1`, `D2`, `D3` are general-purpose on C3/C6 and support `INPUT_PULLUP`.

### **Boot/Strap Pin Analysis (C3 notes)**
- Avoid `D8` (strap: GPIO8) for buses.
- Avoid `D9` (GPIO9) for I2C — tied to BOOT button; OK only as a simple input.
- Avoid `D6` for critical outputs (it’s UART0 TX at boot) — OK for LED.

### **Pin Capability Summary**

| GPIO | Type | Function | Capability | Boot Conflict |
|------|------|----------|------------|---------------|
| 0 | ADC | Battery | ADC1_CH0 | ❌ None ✅ |
| 2 | ADC | Thermocouple | ADC1_CH2 | ❌ None ✅ |
| 16 | PWM | MOSFET gate | LEDC | ❌ None ✅ |
| 17 | Input | Encoder A | INPUT_PULLUP | ❌ None ✅ |
| 19 | I2C | OLED clock | SCL | ❌ None ✅ |
| 20 | I2C | OLED data | SDA | ❌ None ✅ |
| 22 | Input | Encoder SW | INPUT_PULLUP | ❌ None ✅ |
| 23 | Input | Encoder B | INPUT_PULLUP | ❌ None ✅ |

**Result:** All assigned pins are compatible on both XIAO ESP32-C3 and ESP32-C6 when using D/SDA/SCL macros. ✅

---

## ✅ **Design Validation**

- ✅ **All pins accessible** on standard 2×7 socket header
- ✅ **No special assembly** required (no castellated soldering)
- ✅ **Two ADC inputs** for analog sensing (GPIO0, GPIO2 verified as ADC1_CH0, ADC1_CH2)
- ✅ **Hardware I2C** for reliable OLED communication (GPIO19/20 support I2C)
- ✅ **Safe PWM pin** (GPIO16 not a boot/strap pin)
- ✅ **No boot conflicts** (GPIO8, GPIO9, GPIO15 not used)
- ✅ **C3/C6 compatible** (identical pinout)

**Ready for PCB layout!** 🚀
# Final Pin Mapping — XIAO ESP32-C3/C6 (2×7 Header)

**Date:** December 25, 2025 (Revised)  
**Design Goal:** Use ONLY the standard 14-pin (2×7) header on XIAO ESP32-C3/C6 (battery +/− on back)  
**No castellated holes required!** ✅

---

**Compatibility note:** For a final compatibility checklist and board-variant guidance see: [XIAO C3/C6 Compatibility](XIAO-C3-C6-COMPATIBILITY.md)


## 📌 **Complete Pin Assignment (D/SDA/SCL macros)**

### **Left-Side Header (Pins 1-7)**

| Pin # | D Pin | Net Label | Function | Notes | Firmware |
|-------|-------|-----------|----------|-------|----------|
| **1** | D0/A0 | `BAT_ADC` | Battery voltage divider | ADC input | `BAT_PIN` |
| **2** | D1 | `ENC_A` | Encoder channel A | INPUT_PULLUP | `ENC_PIN_A` |
| **3** | D2 | `ENC_B` | Encoder channel B | INPUT_PULLUP | `ENC_PIN_B` |
| **4** | D3 | `ENC_SW` | Encoder switch/button | INPUT_PULLUP | `ENC_PIN_SW` |
| **5** | D4 | `I2C_SCL` | I2C clock (OLED + ADS1115) | Use SCL macro | `OLED_SCL` |
| **6** | D5 | `I2C_SDA` | I2C data (OLED + ADS1115) | Use SDA macro | `OLED_SDA` |
| **7** | D6 | `RGB_RED` | RGB LED red (optional) | Boot UART chatter OK | `RGB_PIN_RED` |

### **Right-Side Header (Pins 8-14)**

| Pin # | D Pin | Net Label | Function | Notes | Firmware |
|-------|-------|-----------|----------|-------|----------|
| **8** | D7/RX | `RGB_GREEN` | RGB LED green (optional) | RX capable | `RGB_PIN_GREEN` |
| **9** | D8/SCK | — | — | Avoid on C3 (strap) | — |
| **10** | D9/MISO | — | — | Avoid on C3 (BOOT button) | — |
| **11** | D10/MOSI | `MOSFET_GATE` | Heater PWM output | LEDC PWM | `OUTPUT_PIN` |
| **12** | 3V3 | `VCC` | 3.3V power output | — | — |
| **13** | GND | `GND` | Ground | — | — |
| **14** | 5V | `VS+` | 5V USB input | — | — |

---

## 🎯 **Firmware Configuration**

```cpp
// config.h — Final pin assignments (standard 2×7 header only)

// ADC Input (left header)
#define BAT_PIN A0            // D0/A0 — battery monitoring

// PWM Output (right header)
#define OUTPUT_PIN D10        // D10/MOSI — heater MOSFET gate

// Encoder (left header)
#define ENC_PIN_A D1          // Pin 2 left
#define ENC_PIN_B D2          // Pin 3 left
#define ENC_PIN_SW D3         // Pin 4 left

// I2C Bus (shared OLED + ADS1115 Thermocouple)
#define OLED_SDA SDA          // Board default SDA (D5)
#define OLED_SCL SCL          // Board default SCL (D4)
#define USE_ADS1115 1        // Enable ADS1115 I2C thermocouple converter
#define ADS1115_I2C_ADDR 0x48 // ADS1115 I2C address (K-type thermocouple)

// RGB LED Status Indicator (optional)
#define RGB_LED_ENABLED 1     // Set 0 to disable if pins are tight
#define RGB_PIN_RED D6        // D6 (tolerates boot UART chatter)
#define RGB_PIN_GREEN D7      // D7/RX
// Third channel is intentionally unassigned on C3 for safety; consider WS2812.
```

---

## 🔌 **Wiring Connections**

### **U1 (ADS1115 Thermocouple Converter IC, SOIC-8) → U3 (XIAO) via I2C**
- U1 pin 1 (ADDR) → GND (sets I2C address to 0x48)
- U1 pin 2 (GND) → GND (U3 pin 13)
- U1 pin 3 (SCL) → **SCL (D4)** via `I2C_SCL` net (shared with OLED)
- U1 pin 4 (SDA) → **SDA (D5)** via `I2C_SDA` net (shared with OLED)
- U1 pin 5 (AIN0) → Thermocouple input (K-type)
- U1 pin 6 (AIN1) → Thermocouple input (K-type, differential)
- U1 pin 7 (ALRT) → Not used (floating or GND)
- U1 pin 8 (VDD) → 3.3V (U3 pin 12)
- Add 100nF bypass cap from VDD to GND near U1

### **Battery Divider → U3 (XIAO)**
- Battery voltage divider → **D0/A0** (U3 pin 1) via `BAT_ADC` net

### **Q1 (MOSFET) → U3 (XIAO)**
- Q1 gate ← **D10** via `MOSFET_GATE` net
- Add 100Ω series resistor + 100kΩ pull-down on gate

### **RGB LED (Status Indicator) → U3 (XIAO)**
- Red channel → **D6** via 220Ω resistor (OK if it flickers at boot)
- Green channel → **D7** via 220Ω resistor
- Common cathode → **GND**
- Optional: switch to single-wire addressable LED (WS2812) on one spare D pin for full RGB

### **ENC1 (Rotary Encoder) → U3 (XIAO)**
- ENC1 terminal A → **D1** via `ENC_A` net
- ENC1 terminal B → **D2** via `ENC_B` net
- ENC1 switch → **D3** via `ENC_SW` net
- Common terminal → GND

### **J4 (OLED Connector) → U3 (XIAO) via I2C**
- J4 SDA → **SDA (D5)** via `I2C_SDA` net (shared with ADS1115)
- J4 SCL → **SCL (D4)** via `I2C_SCL` net (shared with ADS1115)
- J4 VCC → 3.3V (U3 pin 12)
- J4 GND → GND (U3 pin 13)
- **Note:** OLED uses I2C address 0x3C, ADS1115 uses 0x48 → no address conflict

---

## ✅ **Key Advantages of This Mapping**

1. **✅ No castellated holes required** — all pins on standard 2×7 header
2. **✅ Single ADC input available** — GPIO0 for battery monitoring (sufficient)
3. **✅ I2C Thermocouple (ADS1115)** — 3.3V native, shares I2C bus with OLED (no new pins!)
4. **✅ Multi-master I2C** — OLED (0x3C) and ADS1115 (0x48) coexist without conflicts
5. **✅ Hardware I2C on dedicated bus** — SCL/SDA macros (D4/D5) map per board
6. **✅ PWM on safe pins** — D10 (heater), D6/D7 OK for LEDs (D6 chatters at boot)
7. **✅ Encoder on general-purpose GPIOs** — no special requirements
8. **✅ RGB LED status indicator** — Full-color status using 3 remaining GPIO pins
9. **✅ Simpler schematic** — Single ADS1115 IC replaces AD8495 + TLV431 reference
10. **✅ Compatible with C3 and C6** — uses D/SDA/SCL macros (identical header)

---

## 📋 **Schematic Update Checklist**

Before PCB layout:

### **U3 (XIAO Module) Pin Connections:**
- [ ] Pin 1 (D0/A0) → `BAT_ADC` net (from battery divider)
- [ ] Pin 2 (D1) → `ENC_A` net (from encoder A)
- [ ] Pin 3 (D2) → `ENC_B` net (from encoder B)
- [ ] Pin 4 (D3) → `ENC_SW` net (from encoder switch)
- [ ] Pin 5 (D4) → `I2C_SCL` net (to J4 SCL)
- [ ] Pin 6 (D5) → `I2C_SDA` net (to J4 SDA)
- [ ] Pin 7 (D6) → `RGB_RED` net (optional)
- [ ] Pin 8 (D7/RX) → `RGB_GREEN` net (optional)
- [ ] Pin 11 (D10/MOSI) → `MOSFET_GATE` net (to Q1 gate)
- [ ] Pin 12 (3V3) → `VCC` net (power rail)
- [ ] Pin 13 (GND) → `GND` net (ground plane)
- [ ] Pin 14 (5V) → `VS+` net (power input)

### **Add Missing Capacitors:**
- [ ] C6 (100nF) between U2 pin 2 (`REF`) and GND
- [ ] C7 (100nF) between U1 pin 6 (`OUT_ADC`) and GND

### **Verification:**
- [ ] Run Design → Electrical Rules Check (ERC)
- [ ] Verify all nets have labels matching firmware config
- [ ] Verify no floating pins on U1, U2, U3
- [ ] Check that all power pins (VS+, VCC, GND) are properly connected

---

## 🧪 **Testing Procedure**

After PCB assembly:

1. **Power test:**
   - Measure 3.3V on U3 pin 12
   - Measure 5V on U3 pin 14
   - Verify GND continuity on U3 pins 13/16

2. **Reference voltage:**
   - Measure ~1.24V on `REF` net (U2 pin 2)

3. **ADC inputs:**
   - **D0/A0 (Battery):** Should read ~1.65V for 3.7V battery (50% divider)

4. **Encoder:**
   - Rotate encoder: D1/D2 should toggle (check with oscilloscope or LED)
   - Press switch: D3 should go LOW when pressed

5. **I2C OLED:**
   - Upload firmware
   - OLED should initialize and display boot screen
   - If not working: check SDA/SCL signals with logic analyzer

6. **PWM output:**
   - D10 should output PWM signal (~200 Hz, 10-bit)
   - Measure with oscilloscope on Q1 gate

---

## 🔄 **Migration Summary**

### **Changes from Previous Version:**

| Function | Old GPIO | New GPIO | Reason |
|----------|----------|----------|--------|
| Thermocouple ADC | 3 (castellated) → 4 (castellated) | **2 (header)** | Avoid castellated holes |
| Battery ADC | 5 (castellated) | **0 (header)** | Already on header ✅ |
| Encoder A | 0 → 6 (castellated) | **17 (header)** | Free up GPIO0, avoid castellated |
| OLED SDA | 2 → 1 | **20 (header)** | Free up GPIO2 for ADC |
| OLED SCL | 1 | **19 (header)** | Move with SDA |

**Net result:** All 8 active signal pins now use standard 2×7 header!

---

## 📖 **Pin Usage Summary**

**Used Pins:** 7–9 of 11 GPIO pins
- Left header: D0/A0, D1, D2, D3, D4(SCL), D5(SDA), D6(optional)
- Right header: D7(optional), D10 (MOSFET)

**Available Pins:** Depends on RGB usage
- If using 2-channel RGB on D6/D7: free pins include D8, D9 (avoid on C3), or a D-pin freed by moving encoder

**Reserved Pins:**
- Power: 3V3, GND, 5V
- JTAG/Debug: Top header (optional, not in main 2×7)

---

## ⚙️ **GPIO Capability Verification (C3/C6)**

### **ADC Pins (Analog Input)**
- Use `A0` (D0) for battery; maps to ADC1 on both C3/C6.
- Avoid `A3` on ESP32-C3 (ADC2 reliability note from Seeed docs).

### **I2C Pins (Hardware I2C)**
- Use `SCL` (D4) and `SDA` (D5) macros for portability; Arduino core maps them per board.
- 400 kHz supported; add 4.7k pull-ups if OLED module lacks them.

### **PWM Pin (LEDC Output)**
- Use `D10` for MOSFET PWM; safe on C3 (not strap/BOOT), supports LEDC on C3/C6.

### **Digital Input Pins (Encoder)**
- `D1`, `D2`, `D3` are general-purpose on C3/C6 and support `INPUT_PULLUP`.

### **Boot/Strap Pin Analysis (C3 notes)**
- Avoid `D8` (strap: GPIO8) for buses.
- Avoid `D9` (GPIO9) for I2C — tied to BOOT button; OK only as a simple input.
- Avoid `D6` for critical outputs (it’s UART0 TX at boot) — OK for LED.

### **Pin Capability Summary**

| GPIO | Type | Function | Capability | Boot Conflict |
|------|------|----------|------------|---------------|
| 0 | ADC | Battery | ADC1_CH0 | ❌ None ✅ |
| 2 | ADC | Thermocouple | ADC1_CH2 | ❌ None ✅ |
| 16 | PWM | MOSFET gate | LEDC | ❌ None ✅ |
| 17 | Input | Encoder A | INPUT_PULLUP | ❌ None ✅ |
| 19 | I2C | OLED clock | SCL | ❌ None ✅ |
| 20 | I2C | OLED data | SDA | ❌ None ✅ |
| 22 | Input | Encoder SW | INPUT_PULLUP | ❌ None ✅ |
| 23 | Input | Encoder B | INPUT_PULLUP | ❌ None ✅ |

**Result:** All assigned pins are compatible on both XIAO ESP32-C3 and ESP32-C6 when using D/SDA/SCL macros. ✅

---

## ✅ **Design Validation**

- ✅ **All pins accessible** on standard 2×7 socket header
- ✅ **No special assembly** required (no castellated soldering)
- ✅ **Two ADC inputs** for analog sensing (GPIO0, GPIO2 verified as ADC1_CH0, ADC1_CH2)
- ✅ **Hardware I2C** for reliable OLED communication (GPIO19/20 support I2C)
- ✅ **Safe PWM pin** (GPIO16 not a boot/strap pin)
- ✅ **No boot conflicts** (GPIO8, GPIO9, GPIO15 not used)
- ✅ **C3/C6 compatible** (identical pinout)

**Ready for PCB layout!** 🚀
