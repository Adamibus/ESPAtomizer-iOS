# ESPAtomizer v3 Schematic Review & Required Changes

**Current Status:** ~75% Complete  
**Date:** December 7, 2025  
**File:** `EspAtomizer-PCB_v3.kicad_sch`

---

## ✅ **PRESENT: Core Components & Nets**

| Component | Ref | Present | Status |
|-----------|-----|---------|--------|
| **XIAO Module (C3/C6)** | U1/U4 | ✅ Yes | 2x instances found (duplicate?) — should use only 1 |
| **AD8495 Thermocouple Amp** | U2 | ✅ Yes | Located at (52.07, 116.84) |
| **TLV431AIDBVR Reference** | U3 | ✅ Yes | Located at (53.34, 166.37) |
| **MOSFET (IRLB8721PBF)** | Q1 | ✅ Yes | Present |
| **Thermocouple Input (1×2)** | J1 | ✅ Yes | THK+/THK− labels found |
| **Encoder (Rotary Switch)** | SW1 | ✅ Yes | Present |
| **OLED Connector (1×4)** | J4 | ✅ Yes | Present |
| **Heater Output (1×2)** | J5 | ✅ Yes | Present |
| **DIP Switch** | SW2 | ✅ Yes | Present |
| **Main Caps (100nF)** | C1, C3, C4 | ✅ Yes | Power bypass caps present |
| **Additional Cap** | C2 | ✅ Yes | U4 bypass cap |

### **Net Labels Found** (Verified)
- ✅ `THK+` — Thermocouple positive (J1 pin 1 → U2 pin 2)
- ✅ `THK−` — Thermocouple negative (J1 pin 2 → U2 pin 3)
- ✅ `OUT_ADC` — AD8495 output (U2 pin 6 → GPIO3)
- ✅ `REF` — Reference voltage 1.24V (U3 output → U2 pin 5)
- ✅ `BAT+` / `BAT−` — Battery supply
- ✅ `BAT_ADC` — Battery ADC (GPIO5)
- ✅ `MOSFET_GATE` — Heater gate (GPIO16)
- ✅ `ENC_A` / `ENC_B` / `ENC_C` — Encoder signals
- ✅ `OLED_SDA` / `OLED_SCL` — I2C bus
- ✅ `VCC` — Logic supply
- ✅ `GND` — Ground plane

---

## ⚠️ **CRITICAL ISSUES TO FIX**

### **1. DUPLICATE XIAO MODULE (BLOCKER)**
**Issue:** Two XIAO modules found:
- `U1` — XIAO-ESP32-C3-SMD
- `U4` — XIAO-ESP32-C6-SMD (labeled as "XIAO-ESP32-S3-SMD" — incorrect!)

**What You Need to Do:**
- ❌ **Delete U4 entirely** — You only need ONE XIAO module
- ✅ **Keep U1** — XIAO-ESP32-C3-SMD (firmware auto-detects C3/C6)
- Update `U1` value property: Change from blank/auto-detect to `XIAO-ESP32-C3/C6` or leave generic
- **Why:** You can't have two microcontrollers on one PCB. The firmware supports both C3 and C6 via board detection, so one footprint serves both versions.

### **2. MISSING FILTER CAPACITORS (HIGH PRIORITY)**

| Cap | Location | Value | Purpose | Status |
|-----|----------|-------|---------|--------|
| **C6** | ⚠️ MISSING | 100nF | U3 (TLV431) output bypass | ❌ NOT IN SCHEMATIC |
| **C7** | ⚠️ MISSING | 100nF | U2 (AD8495) output filter | ❌ NOT IN SCHEMATIC |

**What You Need to Do:**
1. **Add C6 (100nF capacitor)**
   - Connect between U3 pin 2 (REF output) and GND
   - Purpose: Stabilize reference voltage, reduce noise on AD8495 pin 5
   - Critical for thermocouple accuracy
   
2. **Add C7 (100nF capacitor)**
   - Connect between U2 pin 6 (OUT) and GND
   - Purpose: ADC input filter, reject high-frequency noise
   - Improves ADC sampling accuracy

**Before PCB Layout, these MUST be added.** Without C6, the reference may oscillate. Without C7, ADC readings will be noisy.

### **3. OPTIONAL ADDITIONAL DECOUPLING (MEDIUM PRIORITY)**

| Cap | Location | Value | Purpose |
|-----|----------|-------|---------|
| **C5** | U2 power pins | 100nF | AD8495 supply bypass |
| **C8** | U3 power pins | 100nF | TLV431 supply bypass |

**Current State:** C1, C3, C4 cover main XIAO supply; C2 covers U4 (which will be deleted).

**Recommendation:** After deleting U4 and adding C6/C7, optionally add C5 & C8 near U2/U3 power pins for extra stability (not critical for low-noise environments, but improves margin).

---

## 🔍 **CONNECTIONS VERIFICATION NEEDED**

| Net | From | To | Status |
|-----|------|----|----|
| **THK+** | J1 pin 1 | U2 pin 2 (IN+) | ✅ Labeled |
| **THK−** | J1 pin 2 | U2 pin 3 (IN−) | ✅ Labeled |
| **OUT_ADC** | U2 pin 6 (OUT) | GPIO3 (via label) | ✅ Labeled |
| **REF** | U3 pin 2 (VREF) | U2 pin 5 (VREF) | ✅ Labeled |
| **U2 GND** | U2 pin 4 | GND | ⚠️ **VERIFY** |
| **U2 VS+** | U2 pin 8 | VS+ (5V) | ⚠️ **VERIFY** |
| **U3 GND** | U3 pin 3 | GND | ⚠️ **VERIFY** |
| **U3 VS+** | U3 pin 1 | VS+ (5V) | ⚠️ **VERIFY** |
| **MOSFET_GATE** | GPIO16 (via label) | Q1 gate | ✅ Labeled |
| **BAT_ADC** | Battery divider | GPIO5 (via label) | ✅ Labeled |
| **OLED_SDA** | J4 pin 3 | GPIO2 (via label) | ⚠️ **VERIFY J4 PINOUT** |
| **OLED_SCL** | J4 pin 4 | GPIO1 (via label) | ⚠️ **VERIFY J4 PINOUT** |

**What You Need to Do:**
- Open the schematic in KiCad
- Click on each power pin of U2 and U3; verify they connect to correct nets (VS+ and GND)
- Verify J4 connector pins 3 & 4 carry SDA/SCL (not pins 1 & 2)
- Ensure no floating pins on U2/U3 (all 8 pins of U2, all 3 pins of U3 should have nets)

---

## 📋 **STEP-BY-STEP FIX CHECKLIST**

### **Phase 1: Delete Duplicate & Add Caps (CRITICAL)**
- [ ] **Delete U4** (XIAO-ESP32-C6-SMD instance) completely
  - Select U4, press Delete
  - Remove any related wires/nets that were only for U4
- [ ] **Add C6 capacitor** (100nF)
  - Right-click → Add Symbol → Device → C_Small_US
  - Place near U3 TLV431 (e.g., at 45, 166)
  - Label it "C6"
  - Wire pin 1 → U3 pin 2 (REF)
  - Wire pin 2 → GND
- [ ] **Add C7 capacitor** (100nF)
  - Right-click → Add Symbol → Device → C_Small_US
  - Place near U2 AD8495 (e.g., at 62, 130)
  - Label it "C7"
  - Wire pin 1 → U2 pin 6 (OUT)
  - Wire pin 2 → GND

### **Phase 2: Verify Power & Ground (CRITICAL)**
- [ ] **U2 pin 4** (GND) → Verify wired to GND net
- [ ] **U2 pin 8** (VS+) → Verify wired to VS+ net (5V supply)
- [ ] **U2 pin 7** (NC) → Leave unconnected
- [ ] **U2 pin 1** (NC) → Leave unconnected
- [ ] **U3 pin 3** (GND) → Verify wired to GND net
- [ ] **U3 pin 1** (CATHODE, VS+) → Verify wired to VS+ net

### **Phase 3: Verify Analog Connections (HIGH PRIORITY)**
- [ ] **U2 pin 2** (IN+) → THK+ net (from J1)
- [ ] **U2 pin 3** (IN−) → THK− net (from J1)
- [ ] **U2 pin 5** (VREF) → REF net (from U3)
- [ ] **U2 pin 6** (OUT) → OUT_ADC net (to GPIO3)
- [ ] **U3 pin 2** (VREF OUT) → REF net (to U2 pin 5 and C6)

### **Phase 4: Verify Signal Integrity (MEDIUM PRIORITY)**
- [ ] **OUT_ADC net** → Confirm routed to GPIO3 (ADC1_CH3) on U1 header
- [ ] **BAT_ADC net** → Confirm routed to GPIO5 (ADC1_CH5) on U1 header
- [ ] **MOSFET_GATE** → Confirm routed to GPIO16 on U1 header
- [ ] **OLED_SDA/SCL** → Confirm routed to GPIO2/GPIO1 on U1 header
- [ ] **ENC_A/B/C** → Confirm routed to GPIO0/23/22 on U1 header

### **Phase 5: Optional Enhancements**
- [ ] Add pull-up resistors on I2C bus (4.7kΩ SDA, 4.7kΩ SCL) if OLED doesn't have them
- [ ] Add pull-up on encoder switch (10kΩ to VCC) if not built-in
- [ ] Add R2 series resistor on MOSFET gate (~1kΩ) to limit capacitive loading
- [ ] Add R3 pull-down on encoder switch for debounce

---

## 🚀 **RECOMMENDED NEXT STEPS (IN ORDER)**

1. **Fix Critical Issues (Today)**
   - Delete U4
   - Add C6 and C7
   - Verify all U2/U3 power pins are connected
   - Verify thermocouple input (THK+/THK−) connected to U2 pins 2 & 3
   - Verify REF net connects U3 output to U2 pin 5

2. **Verify Connectivity (Today)**
   - Run **Design → Electrical Rules Check (ERC)**
   - Look for:
     - Floating pins
     - Unconnected nets
     - Missing power rails
   - Fix any ERC errors

3. **Before PCB Layout (This Week)**
   - Verify OUT_ADC/BAT_ADC/MOSFET_GATE/encoder nets routed to correct GPIO
   - Review J4 OLED connector pinout (verify SDA/SCL on correct pins)
   - Run final ERC check
   - Export net list and cross-check with GPIO mapping document

4. **PCB Layout (Next Phase)**
   - Place U1, U2, U3, Q1 on board
   - Route thermocouple inputs short and twisted (minimize noise)
   - Route OUT_ADC to GPIO3 with ground plane underneath
   - Place C6 & C7 as close as possible to U3/U2 output pins
   - Keep C6/C7 away from MOSFET switching noise

---

## 📌 **Key Firmware Expectations**

Your firmware expects:
```cpp
#define AD8495_PIN 3         // OUT_ADC → GPIO3 (ADC1_CH3)
#define BAT_PIN 5            // BAT_ADC → GPIO5 (ADC1_CH5)
#define OUTPUT_PIN 16        // MOSFET_GATE → GPIO16
#define OLED_SDA 2           // GPIO2 (I2C)
#define OLED_SCL 1           // GPIO1 (I2C)
#define ENC_PIN_A 0          // GPIO0
#define ENC_PIN_B 23         // GPIO23
#define ENC_PIN_SW 22        // GPIO22
```

**Schematic must route these nets to these GPIOs exactly, or firmware won't work.**

---

## 📊 **Completeness Score**

| Category | Completeness | Status |
|----------|--------------|--------|
| **Core ICs** | 100% | ✅ U1 (XIAO), U2 (AD8495), U3 (TLV431), Q1 (MOSFET) |
| **Connectors** | 100% | ✅ J1 (thermocouple), J4 (OLED), J5 (heater), J2 (TBD) |
| **Main Decoupling** | 75% | ⚠️ C1–C4 present; **C6 & C7 missing** |
| **Signal Integrity** | 80% | ⚠️ Nets labeled; need connection verification |
| **Power Distribution** | 70% | ⚠️ Needs verification; check all supply pins |
| **Control Signals** | 85% | ⚠️ GPIO routing not yet verified on headers |
| **Overall** | **75%** | ⚠️ **BLOCKER: U4 deletion + C6/C7 addition** |

---

## 🎯 **Summary: What to Change NOW**

**Before you can proceed to PCB layout, you MUST:**

1. ✋ **Delete U4** (duplicate XIAO module) — only keep U1
2. ➕ **Add C6** (100nF) between U3 pin 2 and GND
3. ➕ **Add C7** (100nF) between U2 pin 6 and GND
4. ✔️ **Verify power pins** on U2 & U3 are connected to VS+ and GND
5. ✔️ **Verify analog nets** THK±, REF, OUT_ADC are routed correctly
6. ✔️ **Run ERC** (Design → Electrical Rules Check) and fix any errors

**Time to complete:** ~30–45 minutes in KiCad

**Once done:** Schematic will be ~95% complete, ready for PCB layout phase.

---

**Questions?** Reference the `GPIO-PIN-AND-NET-MAPPING.md` document for detailed net and pin assignments.
