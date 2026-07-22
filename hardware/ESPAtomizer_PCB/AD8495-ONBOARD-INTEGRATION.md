# AD8495 Onboard Thermocouple Amplifier Integration

## Overview
This document provides comprehensive guidance for integrating the **AD8495 precision thermocouple amplifier** directly onto the ESPAtomizer v3 PCB, replacing the external MAX6675 breakout board.

**Key Benefits:**
- Direct analog output (5 mV/°C) to ADC — no SPI overhead
- Integrated cold-junction compensation (CJC)
- Reduced BOM cost and board space
- Faster temperature response
- Simplified firmware (single ADC read vs. SPI protocol)

---

## Hardware Integration

### Schematic Checklist

**ICs:**
- [ ] **U2: AD8495** (MSOP-8) — Thermocouple amplifier IC
- [ ] **U4: TLV431AIDBVR** (SOT-23-3) — 1.24V precision shunt reference for AD8495

**Decoupling:**
- [ ] **C1–C5** (100nF, 0805) — Bypass capacitors on power pins (VS+, GND)
- [ ] **C6** (100nF, 0805) — Reference bypass capacitor on U4 output
- [ ] **C7** (100nF, 0805) — Output filter capacitor on AD8495 output

**Connectors:**
- [ ] **J1 or J_THERMO** (1×2 header, 0.1" pitch) — Thermocouple input (K-type recommended)
  - Pin 1: Thermocouple + (red lead)
  - Pin 2: Thermocouple − (yellow lead)

**Electrical Parameters:**
- **AD8495 Supply Voltage:** 4.5V to 5.5V (or 4.75V to 5.25V nominal)
- **AD8495 Output Voltage:** 0V to ~5V (0°C to 500°C for K-type)
- **Output Scaling:** 5 mV/°C (e.g., 2.5V @ 0°C, 3.5V @ 200°C)
- **Reference Voltage (U4):** 1.24V ±1%
- **ADC Input Pin:** GPIO3 (ADC1_CH3, recommended) or GPIO5 (ADC1_CH5)
- **ADC Reference:** 3.3V (XIAO module)

---

## Schematic Circuit Details

### AD8495 Pinout (MSOP-8)
```
       (Top View)
    GND |1    8| VS+
IN+ RTC |2    7| SENSE
IN- RTC |3    6| OUTPUT
     NC |4    5| VREF
```

**Pin Assignments:**
- **Pin 1 (GND):** Ground (typically tied to VS−)
- **Pin 2 (IN+):** Positive thermocouple input (through R3)
- **Pin 3 (IN−):** Negative thermocouple input (through R4)
- **Pin 4 (NC):** No Connect
- **Pin 5 (VREF):** Reference input from U4 output (1.24V)
- **Pin 6 (OUTPUT):** Analog output to ADC (through R5 + C7 filter)
- **Pin 7 (SENSE):** Feedback/sense pin (typically tied to OUTPUT or left floating)
- **Pin 8 (VS+):** Positive supply (4.75V–5.25V)

### Reference Circuit (U4: TLV431AIDBVR)
```
VS+ (5V)
  |
  +--- C6 (100nF) ---+
  |                   |
  R (high impedance) |
  |                   |
  +--- U4 ---+--- VREF (to U2 pin 5)
             |
            GND
```

**TLV431 Pinout (SOT-23-3):**
- **Pin 1 (A):** Anode (to 1.24V reference output)
- **Pin 2 (K):** Cathode (to GND through sense resistor)
- **Pin 3 (REF):** Reference input (tied to A for 1.24V output)

### Simplified Circuit (Non-Harsh Environment)
```
Thermocouple Connector
      |
      J1 (1x2 Header)
      |
      +--- IN+ (U2 pin 2)
      |
      +--- IN− (U2 pin 3)

AD8495 Output Filter
OUTPUT (U2 pin 6)
      |
      +---+--- ADC GPIO (3 or 5)
          |
         C7 (100nF)
          |
         GND
```

---

## Bill of Materials (AD8495 Circuit Only)

| Qty | Reference | Value | Package | Notes |
|-----|-----------|-------|---------|-------|
| 1 | U2 | AD8495 | MSOP-8 | Thermocouple amplifier |
| 1 | U4 | TLV431AIDBVR | SOT-23-3 | Precision reference |
| 1 | J1 | Conn_01x02 | 0.1" Header | Thermocouple input |
| 5 | C1–C5 | 100nF | 0805 | Decoupling caps |
| 1 | C6 | 100nF | 0805 | Reference bypass |
| 1 | C7 | 100nF | 0805 | Output filter |

**Estimated Cost:** $3–$5 (U.S. pricing)

---

## PCB Layout Guidelines

### Placement
1. **U2 (AD8495):** Place near the ESP32 module; close to ADC GPIO pin routing
2. **U4 (TLV431):** Place within 5mm of U2 pin 5 (VREF input)
3. **Thermocouple Connector (J1):** Place at PCB edge for easy external connection

### Routing
- **Input traces (IN+, IN−):** Keep short (<10mm); use ground plane reference layer
- **Output trace (OUTPUT → ADC):** Run close to ground plane; minimize coupling to switching signals
- **VREF trace (U4 → U2 pin 5):** Keep short; separate from high-frequency signals
- **Ground:** Use solid GND plane; connect all component grounds and power grounds at a single point

### Via Placement
- Place vias near ground pins to minimize inductance
- Use multiple vias (2–3) for power/ground connections

### Decoupling Capacitor Placement
- C1, C2 (100nF): within 5mm of U2 power pins (VS+, GND)
- C3, C4 (100nF): within 5mm of U4 power pins
- C6 (100nF): within 5mm of U4 output (VREF)
- C7 (100nF): at ADC GPIO pin for output filter

---

## Firmware Implementation

### Configuration (`config.h`)

Add or update AD8495 macros:
```cpp
// AD8495 Thermocouple Amplifier Configuration
#define USE_AD8495 1              // Enable AD8495 reads (disable USE_MAX6675)
#define USE_MAX6675 0             // Disable MAX6675 SPI code
#define AD8495_PIN 3              // GPIO3 (ADC1_CH3) for analog output
#define AD8495_SAMPLES 16         // Averaging samples for noise reduction
#define AD8495_VREF_MV 3300       // 3.3V ADC reference (XIAO)
```

### Firmware File (`ad8495.h`)

Create a new header file for AD8495 reads:

```cpp
#ifndef AD8495_H
#define AD8495_H

#include "config.h"

#if USE_AD8495

// AD8495 Constants
const float AD8495_SCALING_MV_PER_COUNT = (float)AD8495_VREF_MV / 4095.0;  // 12-bit ADC
const float AD8495_MV_PER_CELSIUS = 5.0;                                   // 5 mV/°C
const float AD8495_ZERO_CELSIUS_MV = 2500.0;                              // 2.5V @ 0°C

// Temperature in millivolts from raw ADC count
float readThermocoupleTemp_mV() {
    uint32_t raw = 0;
    for (int i = 0; i < AD8495_SAMPLES; i++) {
        raw += analogRead(AD8495_PIN);
    }
    raw /= AD8495_SAMPLES;
    return raw * AD8495_SCALING_MV_PER_COUNT;
}

// Temperature in Celsius from AD8495 output
float readThermocoupleTemp_C() {
    float mV = readThermocoupleTemp_mV();
    // Offset: 2.5V @ 0°C, 5 mV/°C gain
    float temp_C = (mV - AD8495_ZERO_CELSIUS_MV) / AD8495_MV_PER_CELSIUS;
    return temp_C;
}

// Temperature with error bounds (useful for diagnostics)
struct TempReading {
    float celsius;
    float millivolts;
};

TempReading readThermocoupleWithDiagnostics() {
    TempReading reading;
    reading.millivolts = readThermocoupleTemp_mV();
    reading.celsius = (reading.millivolts - AD8495_ZERO_CELSIUS_MV) / AD8495_MV_PER_CELSIUS;
    return reading;
}

#endif // USE_AD8495
#endif // AD8495_H
```

### Update Main Sketch (`ESPAtomizer.ino`)

Replace MAX6675 reads with AD8495 calls:

**Old Code (MAX6675 SPI):**
```cpp
#if USE_MAX6675
  temperature = max6675.readCelsius();
#endif
```

**New Code (AD8495 ADC):**
```cpp
#if USE_AD8495
  #include "ad8495.h"
  temperature = readThermocoupleTemp_C();
#endif
```

### Calibration Notes
- **Default Offset:** 0°C maps to 2500 mV; adjust in `ad8495.h` if observed offset exists
- **Gain Trim:** If readings are consistently off by a scale factor, adjust `AD8495_MV_PER_CELSIUS` in firmware
- **Hardware Calibration:** Place AD8495 + thermocouple in 0°C ice bath; measure millivolts on GPIO3 ADC; adjust offset accordingly

---

## Testing & Validation

### Hardware Checkout
1. **Power Supply Check:**
   - Measure VS+ (pin 8): should be 5V ±5%
   - Measure GND (pin 1): should be 0V
   
2. **Reference Voltage Check:**
   - Measure VREF (U2 pin 5): should be ~1.24V
   - Verify U4 output (between pins 1 & 2): ~1.24V

3. **Output Voltage Check (at 25°C ambient, no thermocouple connected):**
   - Measure OUTPUT (U2 pin 6): should be ~2500 mV (±100 mV)
   - With thermocouple at 0°C (ice bath): measure OUTPUT; should be ~2500 mV
   - With thermocouple at 100°C (steam): measure OUTPUT; should be ~3000 mV

### Firmware Testing
```cpp
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    #if USE_AD8495
    Serial.println("AD8495 Thermocouple Amplifier Test");
    #endif
}

void loop() {
    #if USE_AD8495
    float temp_C = readThermocoupleTemp_C();
    float temp_mV = readThermocoupleTemp_mV();
    
    Serial.print("Thermocouple: ");
    Serial.print(temp_C, 2);
    Serial.print("°C (");
    Serial.print(temp_mV, 0);
    Serial.println(" mV)");
    #endif
    
    delay(1000);
}
```

### Test Cases
| Test | Expected Result | Pass/Fail |
|------|-----------------|-----------|
| No thermocouple, 25°C ambient | ~2500 mV, ~0°C displayed | |
| Thermocouple in ice (0°C) | ~2500 mV, ~0°C displayed | |
| Thermocouple in boiling water (100°C) | ~3000 mV, ~100°C displayed | |
| Thermocouple in oven (200°C) | ~3500 mV, ~200°C displayed | |

---

## Troubleshooting

### Issue: ADC reads 0 or always max (4095)
- **Check:** GPIO3 (AD8495_PIN) is correctly routed and not floating
- **Check:** AD8495 OUTPUT pin is connected to GPIO3 with R5 + C7 filter in place
- **Check:** GPIO3 is not used by WiFi or other peripherals

### Issue: Temperature readings are consistently offset
- **Cause:** Reference voltage (U4) is not 1.24V
- **Fix:** Verify TLV431 is correctly wired; check capacitor values
- **Calibration:** Measure actual VREF voltage and update `AD8495_ZERO_CELSIUS_MV` offset

### Issue: Temperature readings are noisy
- **Fix:** Increase `AD8495_SAMPLES` in config.h (e.g., 32 instead of 16)
- **Fix:** Add series resistor (10kΩ) between OUTPUT and GPIO3 to form RC filter

### Issue: Thermocouple not detected (always reads 0°C)
- **Check:** Connector J1 pins are correct; polarity matches thermocouple leads
- **Check:** Protection resistors R3, R4 are not open circuit
- **Check:** Thermocouple wires are not damaged

---

## Next Steps

1. ✅ Add U2 (AD8495) symbol to schematic
2. ⏳ Add U4 (TLV431AIDBVR) symbol and all passive components
3. ⏳ Route thermocouple input connector to IN+/IN− with protection
4. ⏳ Route AD8495 OUTPUT to GPIO3 with R5 + C7 filter
5. ⏳ Create `ad8495.h` firmware file
6. ⏳ Update `config.h` with AD8495 macros and disable MAX6675
7. ⏳ Update `ESPAtomizer.ino` to call AD8495 temperature reads
8. ⏳ Test ADC readings on actual hardware

---

## References

- **AD8495 Datasheet:** https://www.analog.com/media/en/technical-documentation/data-sheets/ad8494_8495_8496_8497.pdf
- **TLV431 Datasheet:** https://www.ti.com/lit/ds/symlink/tlv431.pdf
- **XIAO ESP32-C3 Pin Map:** https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/
- **K-Type Thermocouple Reference:** https://en.wikipedia.org/wiki/Thermocouple#Type_K

---

**Last Updated:** December 7, 2025  
**Status:** Complete — Ready for schematic implementation
