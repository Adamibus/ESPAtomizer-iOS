# Power Filtering Verification Checklist
## ESPAtomizer v3 PCB Layout

**Date:** December 26, 2025  
**Status:** Pre-layout verification guide  
**Purpose:** Verify optimal placement of C1, C3, C5 during PCB layout phase

---

## ✅ Schematic Status: OPTIMIZED

Your power filtering design is **complete and optimized** in the schematic:

| Capacitor | Value | Purpose | Schematic | Status |
|-----------|-------|---------|-----------|--------|
| **C1** | 100nF | ADS1115 (U1) VDD bypass | ✅ Present | Critical |
| **C3** | 100nF | MOSFET (Q1) power bypass | ✅ Present | Important |
| **C5** | 100µF | Bulk battery storage | ✅ Present | Essential |

**No additional capacitors needed.** This three-capacitor design is production-ready.

---

## 📍 PCB Layout Placement Requirements

### **1. C1 Placement (ADS1115 Supply Bypass)**

**Component:** C1, 100nF ceramic capacitor  
**Reference Designator:** C1  
**Voltage Rating:** ≥ 10V  
**Type:** Unpolarized (ceramic X7R recommended)

**Placement Rules:**
```
✓ MUST BE within 5mm of U1 pin 8 (ADS1115 VDD)
✓ Connect one pad directly to U1 VDD net (via short trace < 10mm)
✓ Connect other pad directly to GND plane (via shortest path)
✓ Minimize PCB loop area (high-frequency return path)
✓ Place on same layer as U1 if possible (preferred)

⚠ DO NOT place away from U1 (defeats bypass purpose)
⚠ DO NOT use long winding traces (adds parasitic inductance)
⚠ DO NOT share ground connection with power stage
```

**Schematic Location:** 
- U1 pin 8 (VDD) → C1 pin 1 → C1 pin 2 → GND

**PCB Footprint:** 
- Likely 0805 or 1206 (check schematic or footprint properties)

**Verification During Layout:**
- [ ] Measure distance: U1 center to C1 center < 5mm
- [ ] Trace width on VDD side: ≥ 10mil
- [ ] Trace width on GND side: ≥ 10mil
- [ ] Via to ground plane: < 2mm from C1 GND pad
- [ ] No other traces crossing C1 VDD trace (no stitching)

---

### **2. C3 Placement (MOSFET Power Bypass)**

**Component:** C3, 100nF ceramic capacitor  
**Reference Designator:** C3  
**Voltage Rating:** ≥ 10V  
**Type:** Unpolarized (ceramic X7R recommended)

**Placement Rules:**
```
✓ MUST BE within 10mm of Q1 drain connection (heater output point)
✓ Bypass capacitor for MOSFET switching transients
✓ Connect between Q1 power rail and GND plane
✓ Use short, direct traces (< 15mm)
✓ Place near Q1 on same PCB layer if possible

⚠ DO NOT place far from Q1 (switching noise bypass won't work)
⚠ DO NOT route under high-current drain trace
⚠ DO NOT share GND connection with analog signal ground
```

**Schematic Connection:**
- Battery+ rail → C3 pin 1 → C3 pin 2 → GND

**PCB Footprint:**
- Likely 0805 or 1206 (same size as C1 preferred)

**Verification During Layout:**
- [ ] Measure distance: Q1 center to C3 center < 10mm
- [ ] C3 placed on same side as Q1
- [ ] Drain trace routed through C3 ground first (before main GND)
- [ ] Trace width: ≥ 10mil on both sides
- [ ] C3 not between Q1 and heater connector (keep bypass local)

---

### **3. C5 Placement (Bulk Storage)**

**Component:** C5, 100µF electrolytic/polarized capacitor  
**Reference Designator:** C5  
**Voltage Rating:** ≥ 16V recommended (battery may spike to ~6V)  
**Type:** Electrolytic capacitor (axial or polarized)

**Critical:** Verify polarity!
- Pin 1 (marked +): Connects to BAT+ (battery positive)
- Pin 2 (marked −): Connects to BAT− (battery negative / GND)

**Placement Rules:**
```
✓ MUST BE placed near battery connector (J2)
✓ Can be placed farther from ICs (bulk storage, not bypass)
✓ Connect directly to BAT+ and BAT− nets
✓ Keep away from high-heat components (esp. Q1)
✓ Orientation: Aluminum can pointing away from PCB edges

⚠ VERIFY POLARITY before soldering!
  ✓ Longer leg = + (anode), goes to BAT+
  ✓ Shorter leg = − (cathode), goes to BAT−
  ✓ If reversed: Capacitor will fail catastrophically

⚠ DO NOT place in confined space (can heats up)
⚠ DO NOT route high-current traces over capacitor body
```

**Schematic Connection:**
- BAT+ net → C5 pin 1 (+ leg)
- BAT− net → C5 pin 2 (− leg)

**PCB Footprint:**
- Axial: Radial leads (10mm length typical for 100µF 16V)
- Check component for exact footprint

**Verification During Layout:**
- [ ] C5 placed within 20mm of battery connector J2
- [ ] Polarity clearly marked on silk screen
- [ ] Positive leg (longer) → BAT+ net
- [ ] Negative leg (shorter) → BAT− net / GND
- [ ] Trace width on battery path: ≥ 15mil
- [ ] No high-frequency switching traces nearby

---

## 🔌 Ground Connection Strategy

**All Three Capacitors Must Use a Star Ground Connection:**

```
         U1 (ADS1115)
            VDD pin 8
              |
              C1 (100nF)
              |
         ↓ ↓ (both share this GND connection)
         
     STAR GROUND POINT
     (primary GND junction)
            |
    ┌───────┼───────┐
    |       |       |
   C3      C5    (other GND)
   GND    GND     connections
   
Pattern:
  • C1, C3 bypass capacitors → local GND via short traces
  • Both connect to SAME star point (minimizes impedance)
  • Star point → main GND plane via single via
  • Avoids ground loops and multiple return paths
```

**Implementation:**
1. Create a GND star point near U1 (between C1 and other components)
2. Route C1 GND pad to star point (< 5mm trace)
3. Route C3 GND pad to star point (< 5mm trace)
4. Route star point to main ground plane via large via (30-50mil)
5. Route C5 GND pad directly to battery GND / main plane (separate path OK for bulk cap)

---

## 📊 Frequency Response Verification

After PCB assembly, measure power supply voltage with oscilloscope to verify filtering:

**Test Method:**
1. Connect oscilloscope probe to C1 (VDD side)
2. Run application with normal heater operation
3. Measure AC voltage ripple at VDD

**Expected Results:**
- **Unloaded:** < 50mV peak-to-peak
- **With heater on:** < 100mV peak-to-peak
- **Frequency content:** Dominated by MOSFET switching frequency (~20kHz)

**If ripple is high:**
- [ ] Check C1 placement (must be < 5mm from U1)
- [ ] Verify low-inductance traces (wide traces, short connections)
- [ ] Check solder quality (cold solder joints = high impedance)
- [ ] Consider adding second C1 bypass if needed (parallel placement)

---

## 🔧 Assembly & Quality Checks

Before soldering these capacitors:

### **Pre-Soldering**
- [ ] Verify footprint matches component size (0805, 1206, etc.)
- [ ] C5 polarity marked clearly on silk screen
- [ ] Board has adequate copper clearance around pads (≥ 8mil)
- [ ] No vias under C5 pads (can cause solder wick effects)

### **Post-Soldering (Visual)**
- [ ] All solder joints shiny (not dull or balled)
- [ ] No cold solder joints (grainy appearance)
- [ ] C5 orientation correct (+ leg to BAT+)
- [ ] No solder bridges between pads

### **Post-Soldering (Electrical)**
- [ ] Continuity check: C1 VDD → U1 pin 8 (via C1)
- [ ] Continuity check: C1 GND → star point
- [ ] Continuity check: C3 drain → BAT+ (via C3)
- [ ] Continuity check: C3 GND → star point
- [ ] Resistance check: C5 pins (should show capacitor charging curve on multimeter, not DC short)

---

## ✅ Final Checklist

Before sending PCB to manufacturing:

- [ ] **C1 (100nF)** placed < 5mm from U1 pin 8 ✓
- [ ] **C3 (100nF)** placed < 10mm from Q1 drain ✓
- [ ] **C5 (100µF)** placed < 20mm from battery connector ✓
- [ ] All three capacitors use star ground connection ✓
- [ ] Trace widths ≥ 10mil on all power paths ✓
- [ ] C5 polarity clearly marked on silk screen ✓
- [ ] Via sizes appropriate for current capacity ✓
- [ ] Ground plane is continuous under all three caps ✓
- [ ] No high-frequency switching traces near capacitors ✓
- [ ] Schematic and PCB match (no missing nets) ✓

---

## 📞 Questions?

If any placement issues arise during layout:

1. **C1 won't fit near U1?** → C1 and U1 are in different areas
   - Move C1 as close as possible (within 10mm)
   - Use wider traces (≥ 20mil) to compensate
   - Consider 0603 or 0402 footprint for C1 if space is tight

2. **C3 placement conflicts with heater connector?**
   - Place C3 between Q1 and main GND (not on far side of connector)
   - Shorten traces by rotating Q1 or C3

3. **C5 won't fit near battery connector?**
   - C5 is bulk capacitor, can be placed anywhere on board
   - Prefer near battery connector, but OK anywhere if needed

4. **Still unsure about placement?**
   - Refer back to this document and the frequency response analysis
   - Follow the "star ground" principle
   - When in doubt, place capacitors closer (shorter = better)

---

**Document Created:** December 26, 2025  
**For Board:** EspAtomizer-PCB_v3.kicad_pcb (when populated)  
**Status:** Pre-layout guide — Use during PCB design phase
