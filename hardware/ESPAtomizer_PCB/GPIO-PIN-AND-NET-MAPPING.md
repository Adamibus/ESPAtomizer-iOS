# GPIO Pin & Net Label Mapping for ESPAtomizer v3

Complete reference for PCB schematic net labels and corresponding ESP32-C3/C6 GPIO assignments.

---

## **Quick Reference: GPIO → Net → Function**

| GPIO | ADC | Net Label | Function | Component/Connector | Firmware Macro |
|------|-----|-----------|----------|-------------------|----------------|
| **0** | ADC1_CH0 | BAT_ADC | Battery voltage measurement | Battery divider | `BAT_PIN` |
| **1** | ADC1_CH1 | – | Available | – | – |
| **2** | ADC1_CH2 | OUT_ADC | AD8495 analog output | U1 (thermocouple amp) | `AD8495_PIN` |
| **16** | – | MOSFET_GATE | Heater MOSFET gate (PWM) | Q1 (IRLB8721PBF) | `OUTPUT_PIN` |
| **17** | – | ENC_A | Encoder channel A | ENC1 (rotary encoder) | `ENC_PIN_A` |
| **18** | – | – | Available (SPI_MOSI) | – | – |
| **19** | – | OLED_SCL | I2C Clock (OLED) | J4 (OLED header) | `OLED_SCL` |
| **20** | – | OLED_SDA | I2C Data (OLED) | J4 (OLED header) | `OLED_SDA` |
| **21** | – | – | Not used (non-ADC) | – | – |
| **22** | – | ENC_SW | Encoder switch (button) | ENC1 (rotary encoder) | `ENC_PIN_SW` |
| **23** | – | ENC_B | Encoder channel B | ENC1 (rotary encoder) | `ENC_PIN_B` |

---

## **Power & Ground Nets**

| Net Label | Source | Voltage | Connected To | Notes |
|-----------|--------|---------|--------------|-------|
| **VS+** | Battery or USB | 4.75–5.25V | U2 (pin 8), U3, MOSFET supply | AD8495 & reference supply |
| **GND** | Ground plane | 0V | All ground pins | Common return |
| **VCC** | VS+ or 3.3V reg | 3.3V | XIAO module, OLED, peripherals | Logic supply |

---

## **Analog Nets (ADC Inputs)**

| Net Label | GPIO | ADC Channel | Source | Purpose | Value Range | Firmware |
|-----------|------|-------------|--------|---------|-------------|----------|
| **OUT_ADC** | 2 | ADC1_CH2 | U1 pin 6 (AD8495) | Thermocouple temperature | 0–3.3V (0–500°C) | `readThermocoupleTemp_C()` |
| **BAT_ADC** | 0 | ADC1_CH0 | Battery divider | Battery voltage monitor | 0–3.3V (3.0–4.2V scaled) | `analogRead(BAT_PIN)` |

---

## **Reference Voltage Net**

| Net Label | Source | Destination | Voltage | Purpose |
|-----------|--------|-------------|---------|---------|
| **REF** | U3 pin 1 (TLV431 anode) | U2 pin 5 (AD8495 VREF) | 1.24V ±1% | Thermocouple amp reference |

---

## **Thermocouple Input Nets**

| Net Label | Connector | Pin | Direction | Destination | Notes |
|-----------|-----------|-----|-----------|-------------|-------|
| **THK+** | J1 (1×2 header) | Pin 1 | Input | U2 pin 2 (IN+) | Red thermocouple wire |
| **THK−** | J1 (1×2 header) | Pin 2 | Input | U2 pin 3 (IN−) | Yellow thermocouple wire |

---

## **Heater Output Net**

| Net Label | Source | Destination | Purpose | Notes |
|-----------|--------|-------------|---------|-------|
| **MOSFET_GATE** | GPIO16 (OUTPUT_PIN) | Q1 gate | Heater on/off control | PWM @ 200 Hz, 10-bit resolution |

---

## **I2C Nets (OLED Display)**

| Net Label | GPIO | Function | Connector | Destination | Notes |
|-----------|------|----------|-----------|-------------|-------|
| **OLED_SDA** | 20 | I2C Data | J4 (1×4 header) | OLED module SDA | Address 0x3C |
| **OLED_SCL** | 19 | I2C Clock | J4 (1×4 header) | OLED module SCL | ~400 kHz |

---

## **Encoder Nets**

| Net Label | GPIO | Function | Connector | Destination | Notes |
|-----------|------|----------|-----------|-------------|-------|
| **ENC_A** | 17 | Channel A | ENC1 | Rotary encoder A | 2 edges/detent |
| **ENC_B** | 23 | Channel B | ENC1 | Rotary encoder B | Orthogonal to A |
| **ENC_SW** | 22 | Switch (button) | ENC1 | Rotary encoder SW | Active low, debounce 50ms |

---

## **Connector Pinout Reference**

### **J1: Thermocouple Input (1×2 Header)**
```
[1]—THK+———→ U2 pin 2 (IN+)
[2]—THK−———→ U2 pin 3 (IN−)
```
**Typical wiring:** K-type thermocouple (red = +, yellow = −)

### **J4: OLED Display (1×4 Header)**
```
[1]—VCC———→ OLED VDD
[2]—GND———→ OLED GND
[3]—OLED_SDA———→ OLED SDA (GPIO 2)
[4]—OLED_SCL———→ OLED SCL (GPIO 1)
```

### **J5: Heater Output (1×2 Header)**
```
[1]—+5V or Battery+
[2]—MOSFET Drain (Q1 pin 2) → Heater Load
```

---

## **XIAO Module Left-Row Pin Mapping**

XIAO ESP32-C3/C6 are pin-compatible. Left-row header pads (footprint):

| Pad | GPIO | Label | Current Use |
|-----|------|-------|------------|
| Pad 1 | GPIO 0 | D0 | BAT_ADC (battery voltage, ADC1_CH0) |
| Pad 2 | GPIO 1 | D1 | Available |
| Pad 3 | GPIO 2 | D2 | OUT_ADC (thermocouple, ADC1_CH2) |
| Pad 4 | GPIO 21 | D3 | ~~Not used~~ (non-ADC on C3/C6) |
| Pad 5 | GPIO 22 | D4/SDA | ENC_SW (encoder switch) |
| Pad 6 | GPIO 23 | D5/SCL | ENC_B (encoder B) |
| Pad 7 | GPIO 16 | D6 | MOSFET_GATE (heater PWM) |

**Right-row header pads:**

| Pad | GPIO | Label | Current Use |
|-----|------|-------|------------|
| Pad 8 | GPIO 17 | D7/RX | ENC_A (encoder channel A) |
| Pad 9 | GPIO 19 | D8/SCK | OLED_SCL (I2C clock) |
| Pad 10 | GPIO 20 | D9/MISO | OLED_SDA (I2C data) |
| Pad 11 | GPIO 18 | D10/MOSI | Available |
| Pad 12 | — | 3V3 | Power supply |
| Pad 13 | — | GND | Ground |
| Pad 14 | — | 5V | USB power |

**All pins use standard 2×7 header — no castellated holes required!** ✅

---

## **Firmware Config.h Macros Summary**

```cpp
// Encoder pins (standard header)
#define ENC_PIN_A 17         // GPIO17 (D7/RX) right-side header
#define ENC_PIN_B 23         // GPIO23 (D5/SCL) left-side header
#define ENC_PIN_SW 22        // GPIO22 (D4/SDA) left-side header

// OLED I2C (standard header)
#define OLED_SDA 20          // GPIO20 (D9/MISO) right-side header
#define OLED_SCL 19          // GPIO19 (D8/SCK) right-side header

// Output (heater, standard header)
#define OUTPUT_PIN 16        // GPIO16 (D6) left-side header

// Battery ADC (standard header)
#define BAT_PIN 0            // GPIO0 (D0, ADC1_CH0) left-side header

// AD8495 Thermocouple (standard header)
#define AD8495_PIN 2         // GPIO2 (D2, ADC1_CH2) left-side header
#define AD8495_SAMPLES 16    // Averaging samples
#define AD8495_VREF_MV 3300  // 3.3V reference
```

---

## **Net Routing Checklist**

- [ ] **OUT_ADC (GPIO2):** U1 pin 6 → GPIO2 (pin 3 left header) with 100nF filter cap (C7)
- [ ] **BAT_ADC (GPIO0):** Battery divider net → GPIO0 (pin 1 left header)
- [ ] **REF (1.24V):** U2 output → U1 pin 5 with 100nF bypass cap (C6)
- [ ] **THK+/THK−:** J1 connector → U1 pins 2 & 3 (direct connection, no series R)
- [ ] **MOSFET_GATE:** GPIO16 (pin 7 left header) → Q1 gate with PWM signal
- [ ] **ENC_A/B/SW:** ENC1 encoder → GPIO17 (pin 8 right) / GPIO23 (pin 6 left) / GPIO22 (pin 5 left)
- [ ] **OLED_SDA/SCL:** J4 header → GPIO20 (pin 10 right) / GPIO19 (pin 9 right) I2C bus
- [ ] **VS+ & GND:** Power distribution to all ICs

---

## **Testing Procedure**

1. **Verify power supply:** VS+ should be 5V ±5%, GND at 0V
2. **Check reference:** REF net should read ~1.24V on multimeter
3. **Test ADC GPIO2:** Should read 0–3.3V with thermocouple attached (room temp ~2.5V)
4. **Test ADC GPIO0:** Should read proportional to battery voltage
5. **Verify encoder:** ENC_A (GPIO17)/ENC_B (GPIO23)/ENC_SW (GPIO22) should toggle with rotation/press
6. **Check I2C:** OLED display should initialize on GPIO20 (SDA) / GPIO19 (SCL)
7. **PWM output:** GPIO16 should deliver PWM signal to Q1 gate

---

## **Common Wiring Errors**

| Error | Symptom | Fix |
|-------|---------|-----|
| THK+/THK− reversed | Negative temperature readings | Swap J1 pins 1 & 2 |
| BAT_ADC not connected | Battery monitoring fails (NaN) | Check GPIO5 routing from divider |
| OUT_ADC missing C7 | Noisy temperature readings | Add 100nF cap from U2 pin 6 → GND |
| REF missing C6 | Thermocouple calibration off | Add 100nF cap from U3 → GND |
| MOSFET_GATE not routed | Heater never turns on | Check GPIO16 connects to Q1 gate |
| Encoder pins swapped | Rotation direction inverted | Adjust `ENC_DIR` macro in firmware |

---

## **Reference Documents**

- **AD8495 Datasheet:** https://www.analog.com/media/en/technical-documentation/data-sheets/ad8494_8495_8496_8497.pdf
- **TLV431 Datasheet:** https://www.ti.com/lit/ds/symlink/tlv431.pdf
- **XIAO ESP32-C3 Pinout:** https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/
- **IRLB8721 Datasheet:** https://www.infineon.com/dgdl/irlb8721pbf.pdf?fileId=5546d462533600a40153559ca5652712

---

**Last Updated:** December 7, 2025  
**Status:** Complete — Ready for PCB layout and validation
