# ESPAtomizer PCB v3 — ADS1115 Migration Quick Reference

## 🔄 What Changed?

| Item | Old (AD8495) | New (ADS1115) |
|------|--------------|---------------|
| Thermocouple IC | AD8495 (analog amp) | ADS1115 (I2C ADC) |
| Reference IC | TLV431 (required) | None (internal ref) |
| Filter Caps | C6, C7 | C5 only |
| Power Need | 4.5-30V ❌ | 3.3V ✅ |
| GPIO Used | 9 (GPIO2 ADC) | 8 (GPIO2 unused) |
| Interface | ADC (GPIO2) | I2C (GPIO19/20) |
| I2C Sharing | — | OLED + ADS1115 ✅ |

---

## 📌 Firmware Config Changes

### In `config.h`, REPLACE:
```cpp
// OLD: AD8495 ADC-based thermocouple
#define USE_AD8495 1
#define AD8495_PIN 2
#define AD8495_SAMPLES 16
#define AD8495_VREF_MV 3300
```

### WITH:
```cpp
// NEW: ADS1115 I2C-based thermocouple
#define USE_ADS1115 1
#define ADS1115_I2C_ADDR 0x48
#define ADS1115_CHANNEL 0
#define ADS1115_SAMPLES 16
#define ADS1115_GAIN GAIN_ONE
```

### GPIO Definitions (UNCHANGED):
```cpp
#define BAT_PIN 0              // Still GPIO0, battery ADC
#define OUTPUT_PIN 16          // Still GPIO16, heater PWM
#define ENC_PIN_A 17           // Still GPIO17, encoder A
#define ENC_PIN_B 23           // Still GPIO23, encoder B
#define ENC_PIN_SW 22          // Still GPIO22, encoder switch
#define OLED_SDA 20            // Still GPIO20, I2C shared
#define OLED_SCL 19            // Still GPIO19, I2C shared
// GPIO2 is now UNUSED (was AD8495_PIN)
```

---

## 🔌 Schematic Changes Required

### Delete:
- [ ] U2 (TLV431 reference IC)
- [ ] C6 (100nF bypass on U2)
- [ ] C7 (100nF filter on U1 OUT)

### Replace:
- [ ] U1: AD8495 (MSOP-8) → **ADS1115 (SOIC-8)**

### Add:
- [ ] C5 (100nF bypass cap, U1 VDD to GND)

### Wire U1 (ADS1115) SOIC-8:
```
Pin 1 (ADDR)  → GND              (sets I2C address 0x48)
Pin 2 (GND)   → GND              (ground)
Pin 3 (SCL)   → GPIO19           (I2C clock, shared OLED)
Pin 4 (SDA)   → GPIO20           (I2C data, shared OLED)
Pin 5 (AIN0)  → J1 pin 1         (thermocouple +)
Pin 6 (AIN1)  → J1 pin 2         (thermocouple −)
Pin 7 (ALRT)  → GND or float     (alert pin, unused)
Pin 8 (VDD)   → 3.3V + C5 cap    (power + 100nF to GND)
```

---

## 📦 Files Added to Firmware

### New Files:
- ✅ `ESPAtomizer/ads1115_driver.h` — Header file
- ✅ `ESPAtomizer/ads1115_driver.c` — Implementation
- ✅ `ESPAtomizer/ads1115_integration_example.cpp` — Usage example

### Updated Files:
- ✅ `ESPAtomizer/config.h` — Configuration macros

### Include in Sketch:
```cpp
#include "ads1115_driver.h"
```

---

## 💻 Firmware Integration

### In `setup()`:
```cpp
if (!ads1115_init()) {
    Serial.println("ADS1115 initialization failed!");
    // Handle error: continue or halt
}
```

### In `loop()` (Read Temperature):
```cpp
float temp_c = ads1115_read_temperature();
Serial.printf("Temperature: %.2f°C\n", temp_c);
```

---

## 🧪 Quick Test

### I2C Verification (Arduino IDE Serial Monitor):
```cpp
// Add this debug function to your sketch:
void testADS1115() {
    Serial.println("ADS1115 I2C test:");
    Wire.begin(20, 19);  // SDA=GPIO20, SCL=GPIO19
    
    // Check device address 0x48
    Wire.beginTransmission(0x48);
    int error = Wire.endTransmission();
    
    if (error == 0) {
        Serial.println("✅ ADS1115 found at 0x48");
    } else {
        Serial.println("❌ ADS1115 NOT found (check wiring)");
    }
}
```

---

## 📊 I2C Bus Details

### Shared I2C Bus (GPIO19/20):
```
I2C Master: ESP32-C6 (XIAO)
├─ Device 1: OLED Display (0x3C)
└─ Device 2: ADS1115 ADC (0x48)
```

### No Conflicts:
- OLED expects 0x3C (128×64 SSD1306)
- ADS1115 responds to 0x48 (ADDR pin to GND)
- Both devices share same SCL/SDA lines
- Multi-master I2C ensures proper communication

### I2C Pull-ups:
- If OLED board lacks pull-ups: add 4.7kΩ resistors (GPIO19 and GPIO20 to 3.3V)
- Most OLED boards include pull-ups already (check your module)

---

## 🐛 Troubleshooting

### Problem: "ADS1115 not found at 0x48"
- **Check:** ADDR pin (pin 1) connected to GND?
- **Check:** GPIO19/20 correctly wired?
- **Check:** 3.3V power to ADS1115 pin 8?
- **Check:** I2C pull-ups present on bus?

### Problem: "Temperature readings are zero or wrong"
- **Check:** Thermocouple connected to AIN0/AIN1 (pins 5/6)?
- **Check:** Polarity correct? (Red=+, Yellow=−)
- **Check:** No short circuits on I2C bus?
- **Try:** Call `ads1115_read_raw()` to see if ADC is responding

### Problem: "OLED not working anymore"
- **Check:** I2C bus still shared between OLED and ADS1115
- **Check:** No I2C address conflicts (0x3C ≠ 0x48)
- **Check:** Both devices getting 3.3V power?
- **Try:** Isolate OLED on I2C alone to verify it still works

---

## 📚 Documentation Files Updated

| File | Status | Purpose |
|------|--------|---------|
| `config.h` | ✅ Updated | Firmware configuration |
| `FINAL-PIN-MAPPING-STANDARD-HEADER.md` | ✅ Updated | GPIO pin reference |
| `GPIO-PIN-AND-NET-MAPPING.md` | ✅ NEW | Net and I2C details |
| `README-ESP32C6.md` | ✅ Updated | Wiring guide |
| `SCHEMATIC-REVIEW-CORRECTED.md` | ✅ Updated | Design analysis |
| `ADS1115-MIGRATION-SUMMARY.md` | ✅ NEW | Complete migration guide |
| `ads1115_driver.h` | ✅ NEW | Driver header |
| `ads1115_driver.c` | ✅ NEW | Driver implementation |
| `ads1115_integration_example.cpp` | ✅ NEW | Usage example |

---

## ✅ Pre-Build Checklist

- [ ] config.h updated (USE_ADS1115 defined)
- [ ] ads1115_driver.h/c added to project
- [ ] #include "ads1115_driver.h" in sketch
- [ ] ads1115_init() called in setup()
- [ ] I2C initialized correctly (GPIO20=SDA, GPIO19=SCL)
- [ ] ADS1115 wired to I2C bus (pins 3/4)
- [ ] Thermocouple connected to AIN0/AIN1 (pins 5/6)
- [ ] Power supply 3.3V (no 5V needed)
- [ ] OLED still on same I2C bus (address 0x3C)

---

**Status:** ✅ **READY TO BUILD**

Run `arduino-cli compile` or upload from Arduino IDE. Watch Serial Monitor for initialization messages.
