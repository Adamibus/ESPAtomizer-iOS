# All-SMD PCB Design for ESPAtomizer

**Approach**: Production manufacturing via **Seeed Studio Fusion PCBA** — 100% surface-mount components, direct IC integration (no breakout modules), automated assembly.

---

## Electrical Architecture (All-SMD)

### MCU & Power Management

#### ESP32-C6 (Direct PCB Placement)
- **No socket, no module header** — IC soldered directly to PCB via Seeed Fusion
- **Options**:
  - **Option A** (Recommended): Seeed XIAO ESP32-C6 in SMD-compatible package
    - Contact Seeed for LCSC part code for SMD variant
    - Smallest footprint, fewest additional components
  
  - **Option B**: Generic ESP32-C6 bare IC (LQFP-100 or QFN-package)
    - Source from LCSC: search "ESP32-C6 LQFP100" or "ESP32-C6 QFN"
    - Requires full schematic integration (crystal, decoupling, antenna trace)
    - More complex but maximum flexibility

#### Power Input
- **Battery**: 1S LiPo (3.6–4.2 V) via **SMD JST-GH 1.25mm connector** (2-pin, keyed)
  - Footprint: `Connector_JST:JST_GH_SM02B-GHS-TB_1x2-1MP_P1.25mm_Horizontal`
  - Cost: ~$0.05 per connector (vs. $0.50 for screw terminal)
  - Built-in keyed design prevents reverse polarity

- **Optional**: Hardware battery switch (SPST) on BAT+ for true shutdown
  - Cost: ~$0.10 per board
  - Recommended for safe standby operation

#### Voltage Regulation
- **3.3V from battery**: Direct (most modern components work at 3.3V)
- **Optional 5V→3.3V regulator**: SOT-23-5 package (e.g., LD1117-3.3, AMS1117)
  - Only if connecting legacy 5V sensors
  - Cost: ~$0.20 per board

---

### Power Stage (Low-Side MOSFET Switching)

#### MOSFET (SMD N-Channel, 4–6A rated)
| Aspect | Specification |
|--------|---------------|
| **Package** | SO-8 or DFN-8 (NOT TO-220) |
| **Recommended parts** | AO3400A, BSS138, or equivalent |
| **VGS threshold** | ≤ 2.0 V (logic-level, compatible with 3.3V gate drive) |
| **RDS(on)** | ≤ 10 mΩ @ VGS=3.3V, ID=4A |
| **Maximum rating** | ≥ 30 V, ≥ 10 A (worst-case heater transient) |

#### Gate Drive Circuit (SMD)
| Component | Value/Package | Purpose |
|-----------|---------------|---------|
| **Gate series resistor** | 100 Ω, 0603 SMD | Limits dI/dt to MOSFET gate, prevents oscillation |
| **Gate pull-down** | 100 kΩ, 0603 SMD | Ensures gate LOW when MCU GPIO is high-impedance; reduces EMI |
| **Gate TVS (optional)** | 5V TVS, SOD-523 SMD | Protects gate from transient spikes; only if long traces or noisy environment |

#### Flyback Diode (SMD Schottky)
| Aspect | Specification |
|--------|---------------|
| **Package** | SOD-123 SMD |
| **Type** | Schottky (fast recovery, low forward voltage) |
| **Part examples** | SL4148, SS14, STPS120, or equivalent |
| **Voltage rating** | ≥ 20 V (heater voltage transient spike) |
| **Current rating** | ≥ 4 A average |
| **Placement** | Cathode (bar) to MOSFET Drain; Anode to MOSFET Source |

#### Heater Power Path (High-Current PCB Copper)
| Aspect | Specification |
|--------|---------------|
| **Trace routing** | LiPo(+) → Heater(+) → Heater(−) → MOSFET Drain → MOSFET Source → LiPo(−) |
| **Copper width** | 1–2 mm on inner power plane, OR thick polygon on top layer |
| **Vias** | Multiple vias (0.3 mm hole) between heater traces and bottom power plane |
| **Solder mask** | Clear solder mask from entire heater loop for maximum current capacity |
| **Temperature margin** | Design for < 20°C rise in copper at 4 A (validate with thermal analysis) |

---

### I2C Bus & Decoupling (All-SMD Capacitors)

#### Pull-Up Resistors
- **2× 4.7 kΩ 0603 SMD resistors** on SDA and SCL lines
- Eliminates need for integrated pull-ups on display/ADC modules
- Voltage: 3.3V
- Tolerance: 5% or better

#### Decoupling & Filtering (ALL CERAMIC, no electrolytic)
| Component | Package | Voltage | Placement |
|-----------|---------|---------|-----------|
| **0.1 µF X7R ceramic** | 0603 SMD | 16V | Very close (< 5 mm) to ESP32 VCC pin |
| **0.1 µF X7R ceramic** | 0603 SMD | 16V | Very close to MCP9600 VCC pin |
| **10 µF X5R ceramic** | 1206 SMD | 16V | Battery input, near voltage regulator (if used) |
| **47 µF X5R ceramic** | 1206 SMD | 10V | Optional additional bulk filtering |

**Why ceramic-only**:
- Electrolytic caps can fail under reflow heat (Seeed's furnace reaches 245°C)
- Modern ceramic X5R/X7R materials have low ESR (better filtering than aluminum)
- Reliable for > 10-year lifespan

---

### Display Integration (Direct IC, No Breakout Module)

#### SSD1306 OLED Controller (SMD)
| Aspect | Details |
|--------|---------|
| **Part source** | LCSC: search "SSD1306 LQFP48" or "SSD1306 QFN20" |
| **Package** | LQFP-48 or QFN-20 (choose based on PCB space) |
| **Cost** | ~$0.80 per IC (vs. $3.00 for pre-assembled module) |
| **Supply voltage** | 3.3V from main rail |
| **Interface** | I2C (GPIO22=SCL, GPIO23=SDA) |
| **Design effort** | Moderate: integrate schematic + crystal oscillator + filtering |

#### SSD1306 Supporting Components (SMD)
| Component | Value/Package | Purpose |
|-----------|---------------|---------|
| **Crystal oscillator** | 32.768 kHz, 0.8 ppm, 6-pin SMD | Real-time clock for OLED timing |
| **Crystal load capacitors** | 2× 15 pF, 0603 SMD | Crystal tuning (datasheet values) |
| **Bypass capacitor** | 0.1 µF, 0603 SMD | OLED VCC filtering |
| **I2C pull-ups** | Already included in main I2C section (4.7 kΩ) | Shared with MCP9600 |

#### Display Mechanical
- **OLED panel**: Standard 0.96" SSD1306 128×64 pixels
  - Source: LCSC "OLED 0.96 inch" (pre-assembled panel, soldered to PCB via SMD edge connector)
  - OR: Raw OLED with flex-ribbon connector (more complex mechanical integration)

---

### Thermocouple Interface (Direct IC, No Breakout Module)

#### MCP9600 Thermocouple ADC (SMD)
| Aspect | Details |
|--------|---------|
| **Part source** | LCSC: search "MCP9600 QFN20" or "MCP9600 SSOP20" |
| **Package** | QFN-20 or SSOP-20 |
| **Cost** | ~$0.50 per IC (vs. $3.00 for breakout module) |
| **Supply voltage** | 3.3V from main rail |
| **Interface** | I2C (address 0x60, GPIO22=SCL, GPIO23=SDA) |
| **Design effort** | Moderate: schematic + thermocouple input filtering |

#### MCP9600 Supporting Components (SMD)
| Component | Value/Package | Purpose |
|-----------|---------------|---------|
| **Input filtering capacitor** | 0.1 µF, 0603 SMD | Low-pass filter on thermocouple input (reduces noise) |
| **Bypass capacitor** | 0.1 µF, 0603 SMD | VCC filtering |
| **Address pull-ups (optional)** | 2× 4.7 kΩ, 0603 SMD | If changing I2C address (normally left floating) |

#### Thermocouple Connector (SMD)
| Aspect | Details |
|--------|---------|
| **Connector type** | SMD JST-GH 1.25mm, 2-pin keyed |
| **Footprint** | `Connector_JST:JST_GH_SM02B-GHS-TB_1x2-1MP_P1.25mm_Horizontal` |
| **Wire type** | K-type thermocouple wire (pre-crimped from LCSC or Seeed) |
| **Cost** | ~$0.05 per connector |

---

### Power Output (Heater)

#### Heater Connection (No Connector Component)
- **Direct solder pads on PCB edge** (not a connector)
  - Label (+) and (−) clearly on silkscreen
  - Pads sized for 16+ AWG wire (heater leads)
  - User solders heater wires directly to pads
  - **Cost savings**: Eliminates $0.50 connector
  - **Trade-off**: Not hot-swappable (design for reliability, not serviceability)

#### Heater Safety
- **In-line fuse** (optional): 15–20A blade fuse on battery supply
  - Mounted on PCB or in external battery pigtail
  - Protects against short-circuit current spikes
- **TVS on heater path** (optional): Additional transient protection
  - Only if using very long heater leads (> 1 meter)

---

### Debug Interface (SMD Pogo Pads)

#### UART Test Pads (No Connector)
| Pin | Signal | GPIO |
|-----|--------|------|
| **1** | TX | GPIO21 (or as per firmware) |
| **2** | RX | GPIO20 (or as per firmware) |
| **3** | GND | GND |

**Layout**: 
- Pogo pad pattern (0.75 mm diameter pads)
- 2.54 mm pitch, in triangle or line formation
- Silkscreen labels: TX, RX, GND
- Use reusable **Pogo pin test jig** (3-pin friction contact stick)
- Cost: ~$2–3 per test jig, reusable for unlimited boards

---

## PCB Layout Guidelines (SMD-Optimized)

### Layer Stack-Up (2-Layer Minimum)
| Layer | Purpose |
|-------|---------|
| **Top** | ESP32, MOSFET, gates resistors, decoupling caps, heater traces (thick polygon) |
| **Bottom** | Ground plane (solid) + heater return path (thick traces/polygon) |

**Rationale**: SMD components on top; ground plane on bottom maximizes current capacity and reduces EMI.

### Critical Layout Rules

#### Power Loop (Heater Path)
1. **Shortest path**: LiPo(+) → Heater(+) → Heater(−) → MOSFET → LiPo(−)
2. **Thick copper**: 1–2 mm traces or solid polygon on power layer
3. **Multiple vias**: Connect top → bottom plane at MOSFET drain/source
4. **No guard traces**: Keep small-signal traces (I2C, UART) away from heater loop

#### Ground Plane
- **Solid copper** on bottom layer (lowest impedance return path)
- **Star ground** connection: Battery GND, MOSFET Source, and ESP GND meet at one point
- **Thermocouple GND**: Separate trace to star ground (not through high-current path)

#### Decoupling Capacitors
- **Placement**: Within 5 mm of IC power pin
  - ESP32 VCC: 0.1 µF SMD directly under/beside pin
  - MCP9600 VCC: 0.1 µF SMD directly beside pin
  - Battery input: 10 µF bulk near JST connector

#### Thermocouple Routing
- **Twisted pair**: TC+ and TC− routed as twisted pair (min 5 twists per inch)
- **Shielding** (optional): Aluminum foil + ground strap for very noisy environments
- **Separation**: Keep TC pair at least 10 mm away from heater traces and MOSFET drain node

#### SMD Component Density
- **Avoid clustering**: Spread SMD passives across board for even solder paste coverage
- **Staggered orientation**: Rotate resistor/capacitor orientations 45° to avoid tombstoning
- **Keep-out zones**: No components under ESP32 IC or near connectors

### Via Strategy (SMD Assembly)
- **Via diameter**: 0.3 mm hole, 0.6 mm pad (Seeed Fusion default)
- **Teardrop vias** (optional): Not required for Fusion; auto-routed by Seeed software
- **Via stitching**: Place vias around heater traces to distribute heat to bottom ground plane
- **Do NOT place vias inside solder pads**: Seeed's stencil software will auto-adjust

### Trace Specifications
- **Min trace width**: 0.15 mm (5 mil) for signal traces
- **Min trace clearance**: 0.15 mm (5 mil) between any traces
- **Power traces**: 0.5–2 mm (depending on current)
- **Solder mask clearance**: 0.1–0.15 mm around all pads (auto-generated)

---

## Bill of Materials (SMD-Only)

### Component Count Summary
- **ICs**: 2–3 (ESP32, SSD1306, MCP9600)
- **Passive components**: ~20 (resistors, capacitors, diode)
- **Connectors**: 2–3 SMD JST connectors + pogo pads
- **Total unique parts**: ~25

### LCSC Part Number Template

| Designator | Value | Package | LCSC Part# | Qty | Notes |
|------------|-------|---------|-----------|-----|-------|
| U1 | ESP32-C6 | BGA or QFN | [LCSC code] | 1 | SMD module |
| U2 | SSD1306 | LQFP-48 | [LCSC code] | 1 | OLED controller IC |
| U3 | MCP9600 | QFN-20 | [LCSC code] | 1 | Thermocouple ADC |
| Q1 | AO3400A | SOT-23-6 | [LCSC code] | 1 | N-MOSFET |
| D1 | SL4148 | SOD-123 | [LCSC code] | 1 | Flyback Schottky |
| D2 | BZX55C5V1 | SOD-123 | [LCSC code] | 1 | Gate TVS (optional) |
| R1 | 100 | 0603 | [LCSC code] | 1 | Gate series resistor |
| R2 | 100k | 0603 | [LCSC code] | 1 | Gate pulldown |
| R3, R4 | 4.7k | 0603 | [LCSC code] | 2 | I2C pull-ups |
| C1, C2, C3, C4 | 0.1µF | 0603 | [LCSC code] | 4 | Decoupling caps |
| C5, C6 | 10µF | 1206 | [LCSC code] | 2 | Bulk filtering |
| J1 | JST-GH 1.25mm | SMD 2-pin | [LCSC code] | 1 | Battery connector |
| J2 | JST-GH 1.25mm | SMD 2-pin | [LCSC code] | 1 | Thermocouple connector |
| TP1, TP2, TP3 | Pogo pad | 0.75 mm | [LCSC code] | 3 | UART debug pads |

**Steps**:
1. Search each component type on LCSC (e.g., "ESP32-C6 LQFP100")
2. Filter by SMD package, in-stock status
3. Note part code and unit price
4. Export BOM as `.csv` with LCSC codes

---

## Manufacturing via Seeed Fusion PCBA

### Design Submission Checklist
- [ ] KiCAD project with all SMD footprints from Seeed library
- [ ] PCB design (`*.kicad_pcb` file) passing DRC for SMD
- [ ] BOM in `.csv` format with LCSC part codes and quantities
- [ ] PCB Gerber files (auto-generated by KiCAD)
- [ ] Design review for solder mask, silkscreen, test pads

### Assembly Service Options
| Option | Cost | Details |
|--------|------|---------|
| **PCBA (Full assembly)** | Setup $25–50 + $0.08–0.15/board | Seeed solders all components; recommended |
| **PCB only** | $2–4/board | Bare PCB shipped; you hand-solder components (not recommended for SMD) |
| **Stencil included** | Included with PCBA | Reusable metal stencil for future manual assembly |

### Quote & Ordering Process
1. Upload KiCAD files to https://fusion.seeedstudio.com/
2. Select "PCBA" option (not bare PCB)
3. Upload BOM (`.csv`)
4. Seeed software auto-matches components to LCSC
5. Review quoted assembly cost
6. Accept & pay
7. Lead time: 7–14 days (component + assembly)

### Cost Estimate (100-unit run)
| Item | Cost |
|------|------|
| PCB (2-layer, small): 100 × $1.50 | $150 |
| Assembly setup fee | $35 |
| Assembly labor: 100 × $0.12/board | $12 |
| Component cost (LCSC BOM, ~25 parts): 100 × $1.20 | $120 |
| **Total** | **$317** |
| **Per-unit cost** | **$3.17** |

---

## Testing & Validation

### Factory Testing (Included with Fusion PCBA)
- **AOI (Automated Optical Inspection)**: Checks solder joints, component placement
- **Continuity testing**: Verifies no shorts on power planes
- **Visual inspection**: Human review for rework/defects
- **Rework**: Minor defects (cold solder joints) fixed at no charge

### Post-Assembly Testing (User)
1. **Visual inspection**: Check all connectors seated, no solder blobs
2. **Power-up test**: 
   - Connect battery via JST connector
   - Measure 3.3V on VCC rail (multimeter)
   - Measure ESP32 GPIO outputs with scope (should toggle on firmware load)
3. **I2C scan**: Firmware should detect OLED (address 0x3C) and MCP9600 (address 0x60)
4. **Heater functional test** (with disconnected load):
   - Firmware enables PWM gate drive
   - Measure MOSFET gate voltage with scope (should swing 0–3.3V)
   - Heater should draw expected current when connected

---

## References & Resources

- **Seeed Fusion PCBA**: https://fusion.seeedstudio.com/
- **LCSC Electronics**: https://lcsc.com/
- **Seeed KiCAD Library (GitHub)**: https://github.com/SeeedStudio/KiCad_Lib_LCSC
- **SSD1306 Datasheet**: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
- **MCP9600 Datasheet**: https://ww1.microchip.com/en-US/product/MCP9600
- **Design Rules**: https://wiki.seeedstudio.com/Fusion_PCB_Capability/

---

**Last Updated**: 2026-06-17  
**Status**: All-SMD design specification complete  
**Next Phase**: KiCAD schematic design, layout, DFM check
