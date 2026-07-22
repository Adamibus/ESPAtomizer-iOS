# Seeed Studio Fusion Open Parts Library Integration — All-SMD Design

## Overview

This document describes the ESPAtomizer hardware design evolution to **100% surface-mount (SMD) manufacturing** with **direct PCB placement** of Seeed Studio modules and **full Seeed Fusion assembly service** integration. This approach delivers:
- Professional production-ready PCBs with component assembly included
- Significantly lower per-unit costs at scale
- Seeed Fusion handles all reflow and component placement
- Direct sourcing from LCSC (Seeed's parts distributor)

### Design Philosophy: "Fusion-First"
All components are selected for SMD compatibility and automatic assembly via Seeed Fusion PCBA (PCB Assembly) service.

## Benefits

- **100% Automated Assembly**: Seeed Fusion handles PCB soldering, component placement, testing — no manual soldering required
- **Unified sourcing**: All components sourced and verified by Seeed/LCSC; no third-party supplier fragmentation
- **Direct PCB-placed ESP32**: Seeed ESP32 modules available in SMD format for direct pick-and-place on PCB
- **Cost scaling**: Per-unit cost drops significantly with volume; professional manufacturing at prototype prices
- **Quality assurance**: Seeed Fusion operates under ISO 9001 standards; AOI (Automated Optical Inspection) on all boards
- **Manufacturing compatibility**: All footprints optimized for Seeed's reflow and automated assembly processes
- **One-step ordering**: Upload KiCAD → Select LCSC parts → Seeed handles PCB + assembly + testing → Ship finished PCBs
- **Community library**: Access to vetted designs optimized for Fusion assembly

## Accessing Seeed Studio's Fusion Open Parts Library

### 1. **Online Library**
- **Seeed Fusion LCSC Part Library**: https://www.seeedstudio.com/parts
- **LCSC Electronics**: https://lcsc.com/ (Seeed's preferred component distributor)
- **Component search**: Search by part number, category, or specs directly

### 2. **KiCAD Integration**
- **EasyEDA** (Seeed's recommended tool): https://easyeda.com/
  - Built-in Seeed footprint library
  - Direct integration with LCSC parts database
  - Auto-links components to footprints
  
- **KiCAD Plugin** (alternative):
  - Plugin: `KiCad_Lib_LCSC` available on GitHub
  - Enables direct LCSC component lookup in KiCAD
  - Synchronizes footprints with LCSC specifications

### 3. **GitHub Repository**
- Seeed's KiCAD library: https://github.com/SeeedStudio/KiCad_Lib_LCSC
  - Clone for offline access
  - Regular updates with new components
  - Contributing guide for custom footprints

## All-SMD Design Strategy (Production-Ready)

### Why All-SMD?
| Aspect | All-SMD (Fusion) | Mixed PTH/SMD | All THT (Socket) |
|---|---|---|---|
| **Assembly cost** | $0.08–0.15 per board | $0.15–0.30 per board | Manual soldering (~$2–5) |
| **PCB size** | Smaller 2-layer, dense | Medium, hybrid layout | Larger, spread out |
| **Reflow solder** | Automated, repeatable | Partial automation | None (hand soldering) |
| **Quality** | ISO 9001 tested, AOI | Partial inspection | Variable |
| **Lead time** | 5–10 business days | 7–14 days | 10–20 days (manual) |
| **Scalability** | Excellent (10–1000 units) | Good (50–500) | Poor (manual labor) |
| **Reliability** | High (factory QA) | Medium | High (if done well) |

### Direct PCB-Placement of Seeed Modules

#### MCU: Seeed ESP32-C6 (SMD Option)
**Replace socket-mounted XIAO with direct PCB placement:**

| Aspect | Socket XIAO | Direct PCB Mount (SMD) |
|---|---|---|
| **Footprint** | 2× 1×7 female sockets + 2×7 pin header on module | Single BGA or edge-connector (Seeed recommends) |
| **Board space** | ~20 mm × 40 mm + socket height | ~15 mm × 30 mm, flush on PCB |
| **Assembly** | Manual (user plugs module) | Automated (Seeed places in reflow) |
| **Cost per board** | $1.50 + socket cost (assembly not included) | $0.50 (assembly included in Fusion fee) |
| **Solder reliability** | Connector intermittency risk | Robust reflow joint, validated |
| **Serviceability** | Module swappable | Not field-replaceable (design for reliability) |

**Recommended**: Use Seeed's **SMD-compatible ESP32-C6 breakout module** or **direct BGA** if available in LCSC. Contact Seeed for exact SMD SKU.

#### Power Stage (All SMD)
| Component | SMD Package | LCSC Search | Notes |
|---|---|---|---|
| N-MOSFET (4–6 A) | SO-8 or DFN-8 | "MOSFET N-channel 30V 10A SOT23-6" | Examples: AO3400A, BSS138 (lower current); upgrade to 5-A class SOT-23 for better headroom |
| Flyback Diode | SOD-123 or 0603 | "Schottky diode 20V 2A SOD123" | 1N5819 equivalent in SOD-123 package; fast recovery preferred |
| Gate resistor (100 Ω) | 0603 or 0805 | "100 Ohm resistor 0603" | Standard value, abundant LCSC stock |
| Gate pulldown (100 kΩ) | 0603 or 0805 | "100k resistor 0603" | Standard value |
| Gate TVS (optional) | SOD-523 or SOD-123 | "TVS diode 5V SOD523" | For noise immunity on long traces |

#### I2C Bus & Decoupling (All SMD)
| Component | SMD Package | LCSC Part# Type | Rationale |
|---|---|---|---|
| I2C pull-up (4.7 kΩ) | 0603 or 0805 | "4.7k resistor 0603" | Standard I2C value; 2 resistors (SDA, SCL) |
| Decoupling cap (0.1 µF) | 0603 | "100nF capacitor 0603 16V X7R" | Place near MCU, near each IC supply pin |
| Bulk filtering (10 µF) | 1206 or 1210 | "10µF capacitor 1206 16V X5R" | Input filtering; ceramic preferred for ESR |
| Bulk filtering (47 µF) | 1206 | "47µF capacitor 1206 10V X5R" | Additional input bulk if needed |

#### Surface-Mount Connectors (No Headers!)
**Replace all pin headers with SMD alternatives:**

| Function | Traditional | SMD Alternative | LCSC Example | Rationale |
|---|---|---|---|---|
| **Battery input** | JST-PH 2-pin header | JST-GH 1.25mm or Pogo pins | Search "JST GH 1.25 2pin SMD" | Compact, keyed, solderable |
| **Heater output** | 2-pin 5.08 mm screw terminal | **Pad + via** (no connector!) or SMD barrel jack | Design for direct solder pads on PCB edge | Smallest form factor; user solders wires direct to pads |
| **I2C (if needed)** | 4-pin 2.54 mm header | **No connector** — integrated OLED on PCB OR Pogo test pins | See OLED integration below | Eliminates connector entirely |
| **UART (debug)** | 6-pin 2.54 mm header | **Pogo pin test points** (no connector) | Multiple Pogo pad packs on LCSC | Friction pins for debugging, not soldered connection |

**Recommendation**: Integrate OLED directly on PCB (see below) and use **solder pads on PCB edge** for battery/heater to eliminate most connectors.

#### OLED & Thermocouple: Direct PCB Integration

**Option A: OLED (SSD1306) Soldered Directly to PCB (Recommended)**
- Source breakout IC (SSD1306 chip + crystal) from LCSC in LQFP or QFN package
- Integrate directly into PCB schematic/layout
- Eliminates I2C header + module entirely
- SMD footprints: `SSD1306_QFN20` or similar from Seeed library
- Cost reduction: ~$3 module → ~$0.80 IC + passive components
- **Complexity trade-off**: Requires sourcing raw IC vs. pre-assembled module

**Option B: Pre-assembled SMD OLED Module (Compromise)**
- Some vendors offer SMD-soldered OLED modules (rare but available on LCSC)
- Search LCSC: "OLED 0.96 inch SMD module SSD1306"
- Mount directly to PCB with solder paste, no header

**Thermocouple ADC: MCP9600 SMD Integration**
- MCP9600 IC available in SSOP-20 or QFN-20 package on LCSC
- Integrate directly into PCB (no breakout module)
- Add K-type thermocouple connector as SMD JST-GH 1.25mm or edge pads
- Cost: IC ~$0.50 vs. $3.00 for breakout module

### PCB Assembly Considerations for Seeed Fusion

#### Footprint Standards (All SMD)
- **Resistors/Capacitors**: 0603, 0805 preferred (0402 acceptable; avoid 0201)
- **ICs**: LQFP, QFN, DFN, BGA (no through-hole DIPs)
- **Connectors**: SMD JST, Pogo pads, or no connectors (direct solder pads)
- **No testpoints required** (Seeed AOI handles validation)

#### Design Rules for Seeed Fusion PCBA
- **Stencil apertures**: Let Seeed software auto-generate (do not manually design)
- **Pad-to-via spacing**: Minimum 0.3 mm (Seeed default; check DFM report)
- **Solder mask clearance**: 0.1–0.15 mm around pads (auto-generated)
- **Thermal relief patterns**: Not needed for SMD (unlike via-heavy designs)
- **BOM format**: Generate from KiCAD as `.csv` or `.bom.csv`; LCSC part codes required

#### Assembly Cost Model
- **Setup fee**: ~$25–50 per unique design
- **Per-board assembly**: $0.08–0.15 depending on component count (10–30 components typical)
- **Stencil cost**: Included with first order; reusable for future runs
- **Example 100-board run**: 
  - PCB cost: 100 × $2.00 = $200
  - Assembly: 100 × $0.12 + $35 setup = $47
  - Component cost (from LCSC): ~$40–60 total BOM
  - **Total landed cost: ~$3–4 per assembled board**

---

### Preferred Component Sources (in order)
1. **LCSC Electronics** (https://lcsc.com/) - Seeed's primary distributor
   - Deepest inventory of components
   - Fastest fulfillment for Fusion PCB orders
   - Often included in Fusion PCB ordering pipeline
   
2. **Seeed Studio Direct** (https://www.seeedstudio.com/)
   - Curated selection of popular components
   - Integration with development boards
   - Technical support via community forums

3. **AliExpress/Taobao** (for non-critical components)
   - Fallback for components not in LCSC
   - Longer lead times acceptable
   - Document reasoning in BOM comments

### Components to Standardize

#### Power Stage
| Component | Current Choice | LCSC Part# | Seeed Rating |
|-----------|----------------|-----------|--------------|
| N-MOSFET (low-side) | IRLB8721 / IRLZ44N | Various | ✓ Verified |
| Flyback Diode | 1N5819 / SS14 | Multiple options | ✓ Verified |
| Gate resistor | 100 Ω 0.25W | LCSC code varies | ✓ Standard |
| Gate pulldown | 100 kΩ 0.25W | LCSC code varies | ✓ Standard |

#### I2C Bus (OLED + ADS1115/MCP9600)
| Component | Purpose | LCSC Part# | Notes |
|-----------|---------|-----------|-------|
| 4.7 kΩ pull-up | I2C bus | Multiple options | Use 0.1W+ |
| 0.1 µF capacitor | Decoupling | LCSC code varies | X7R ceramic preferred |
| 10 µF capacitor | Bulk filtering | LCSC code varies | Ceramic or aluminum |

#### Connectors (Available via LCSC)
| Connector Type | Seeed Recommended | LCSC Part# | Notes |
|---|---|---|---|
| Battery (JST-PH) | JST PH 2-pin | Multiple | Safe keyed connector |
| I2C header | 2.54mm 1×4 female socket | Multiple | Standard pitch |
| XIAO socket | 2.54mm 1×7 female socket (x2) | Multiple | Arduino-compatible |
| MCP9600 socket | 2.54mm 1×7 female socket | Multiple | Optional extra pins |
| Screw terminal (heater) | 5.08mm 2-pin | Multiple | High current rated |
| XT30/XT60 (battery pigtail) | Seeed-branded XT connectors | Available | Higher reliability |

## KiCAD Library Configuration

### Adding Seeed Footprints to Your Project

#### Method 1: Clone GitHub Library (Recommended)
```bash
# In your hardware project folder
cd ESPAtomizer-PCB_v3/lib
git clone https://github.com/SeeedStudio/KiCad_Lib_LCSC.git seeed-lcsc
```

#### Method 2: Use EasyEDA Export
- Design in EasyEDA (auto-linked to LCSC footprints)
- Export as KiCAD project (File → Export → KiCAD)
- Footprints auto-populate from Seeed library

### 3. **Manual Library Addition**
Edit `fp-lib-table` in your KiCAD project:
```
(fp_lib_table
  (version 7)
  (lib (name "Seeed_KiCad_Lib_LCSC")(type "KiCad")(uri "${KIPRJMOD}/lib/KiCad_Lib_LCSC")(options "")(descr "Seeed Studio LCSC SMD library"))
  (lib (name "footprints")(type "KiCad")(uri "${KIPRJMOD}/lib/footprints")(options "")(descr "Custom SMD footprints"))
)
```

**Note**: Removed XIAO socket library — using direct SMD ESP32 on PCB instead.

### Footprint Standards for All-SMD ESPAtomizer

#### Direct PCB Integration (No Sockets or Headers)
- **ESP32-C6**: Direct BGA or SMD edge-connector placement on PCB (no socket)
  - Footprint: Seeed-provided ESP32-C6 SMD footprint from LCSC library
  - Soldered in reflow furnace via Seeed Fusion assembly

- **Display (SSD1306 OLED)**: Direct SMD IC on PCB (LQFP-48 or QFN-20)
  - Footprint: `SSD1306_LQFP48` or `SSD1306_QFN20` from Seeed library
  - Integration: Full schematic + PCB layout of OLED controller, not breakout module

- **Thermocouple ADC (MCP9600)**: Direct SMD IC on PCB (QFN-20 or SSOP-20)
  - Footprint: `MCP9600_QFN20` from Seeed library
  - Integration: Full schematic of thermocouple interface

#### Passive Components (All SMD)
- **Resistors**: 0603 or 0805 preferred (0402 acceptable for space-constrained areas)
  - Tolerance: 1% or 5%
  - Power rating: 0.1W or higher
  
- **Capacitors**: 
  - Decoupling (0.1 µF): 0603, X7R ceramic, 16V+
  - Bulk filtering (10–47 µF): 1206, X5R ceramic, 10–16V
  - **No electrolytic capacitors** (prefer ceramic for reflow reliability)

- **Diodes**:
  - Schottky (flyback): SOD-123 or 0603 footprint
  - TVS (optional gate protection): SOD-523 or SOD-123

#### Connectors (SMD or Eliminated)
- **Battery input (JST-GH 1.25mm)**: SMD-soldered keyed connector
  - No pin header alternative
  - Footprint: `Connector_JST:JST_GH_SM04B-GHS-TB_1x4-1MP_P1.25mm_Horizontal`
  
- **Heater output**: **Solder pads only** (no connector component)
  - PCB edge solder pads labeled +, −
  - User solders high-current heater leads directly to PCB

- **Thermocouple input (JST-GH 1.25mm)**: SMD-soldered keyed connector
  - Footprint: `Connector_JST:JST_GH_SM02B-GHS-TB_1x2-1MP_P1.25mm_Horizontal`

- **Debug UART**: Pogo pin test pads (no connector)
  - Footprint: Pogo pad pattern (0.75 mm diameter, 2.54 mm pitch)
  - Used with Pogo pin test jig for programming/debugging

## Component Procurement Workflow

### Step 1: Design in KiCAD with Seeed Library
- Use KiCAD symbol library (matches Seeed footprints)
- Set footprints to Seeed/LCSC library items

### Step 2: Generate Bill of Materials (BOM)
```bash
# Export from KiCAD
Tools → Generate BOM → (use built-in script or external tool)
```

### Step 3: Map to LCSC Part Numbers
- Use LCSC website search for each component
- Document LCSC part codes in KiCAD footprint properties
- Or use KiCAD plugin to auto-populate

### Step 4: Order PCB + Components
- Upload KiCAD files to Seeed Fusion (https://fusion.seeedstudio.com/)
- Select "parts assembly" option
- Auto-import BOM from KiCAD
- Review quoted assembly cost/time
- Order in single transaction

### Step 5: Reflow & Testing
- Components arrive pre-soldered on PCB (if using full assembly)
- Or order bare PCB + BOM for manual assembly

## Updated Component List for ESPAtomizer

### Critical Path Components (Always via LCSC)
| Function | Part | Qty | LCSC Link | Notes |
|---|---|---|---|---|
| MCU | Seeed XIAO ESP32-C6 | 1 | Direct from Seeed | Module-level |
| Heater MOSFET | IRLB8721 or equivalent | 1 | https://lcsc.com/ | Logic-level, 30A+ |
| Thermocouple ADC | MCP9600 module | 1 | LCSC or Seeed | I2C breakout |
| Display | SSD1306 OLED 0.96" | 1 | LCSC or Seeed | I2C 128×64 |
| Gate resistor | 100 Ω 1/4W | 1 | LCSC | Standard part |
| Pull-up resistors | 4.7 kΩ 1/4W | 2–4 | LCSC | I2C standard |
| Decoupling caps | 0.1 µF ceramic | 3–4 | LCSC | Standard |
| Bulk cap | 10 µF ceramic | 1 | LCSC | Input filtering |
| Flyback diode | 1N5819 or SS14 | 1 | LCSC | SMD or PTH |
| Battery connector | JST-PH or XT30 | 1 | Seeed direct | Seeed-branded preferred |
| I2C headers | 2.54mm female socket | 2–4 | LCSC | Standardized |

### Optional Upgrade Components
- **NTC thermistor** (onboard temp sensing): LCSC sourced
- **TVS diode on gate**: LCSC SMD part (0805/1206)
- **RGB status LED**: WS2812B or std RGB from LCSC

## Best Practices for All-SMD Design

### 1. **Design Methodology (EasyEDA Preferred)**
- **Start with EasyEDA** (https://easyeda.com/)
  - Native LCSC parts integration
  - One-click BOM generation with LCSC part codes
  - Automatic footprint mapping for SMD assembly
  - Built-in design rule checker for Fusion manufacturing
  
- **Alternatively: KiCAD + Seeed Library**
  - Clone `KiCad_Lib_LCSC` for all SMD footprints
  - Enable DRC rules for SMD (0.15 mm min trace, 0.3 mm vias)
  - Export BOM as `.csv` with LCSC part codes

### 2. **Component Integration Strategy**
- **Integrate ICs directly on PCB**: No breakout modules for OLED or MCP9600
  - Lower cost (~$0.50/IC vs. $3.00/module)
  - Smaller board footprint
  - Requires sourcing datasheets and designing circuits (one-time effort)

- **Eliminate connectors where possible**: 
  - Battery: SMD JST-GH connector (smallest available)
  - Heater: Direct solder pads on PCB edge (no connector)
  - UART: Pogo test pads (reusable test jig)

- **Standardize passive components**:
  - Only 0603/0805 resistors and capacitors
  - Only ceramic (no electrolytic) capacitors for reflow
  - Minimize part count by using common values (100 Ω, 10 kΩ, 100 kΩ, 4.7 kΩ, etc.)

### 3. **BOM Management for Seeed Fusion**
- **Export from KiCAD**: Tools → Generate BOM
  - Use plugin: `KiCad_Lib_LCSC` includes BOM export template
  - Output format: `.csv` with columns: Designator, Value, Package, LCSC Part#
  
- **Map each component to LCSC part code**:
  - Use LCSC website to find SMD alternatives (not THT)
  - Verify availability and lead time (95%+ should be in-stock)
  - Add LCSC code to KiCAD footprint properties for auto-population

- **Review BOM before ordering**:
  - Min order qty: 1 for all parts (Fusion standard)
  - Check for substitute parts if original unavailable
  - Estimate total component cost on LCSC

### 4. **Manufacturing Specifications**
- **Design rules for Seeed Fusion PCBA**:
  - Min trace width: 0.15 mm (5 mil)
  - Min trace-to-trace clearance: 0.15 mm (5 mil)
  - Via size: 0.3 mm hole, 0.6 mm pad minimum
  - Solder mask clearance: 0.1–0.15 mm (auto-generated)
  
- **Stencil & Solder Paste**:
  - Let Seeed software auto-generate stencil apertures (do NOT manually design)
  - Aperture ratio: Seeed will optimize for reflow
  - Stencil cost: Included in first order; reusable for future runs

- **AOI (Automated Optical Inspection)**:
  - Seeed Fusion includes AOI on all boards (no extra charge)
  - Validates component placement, solder quality, shorts
  - Rework available if defects found

### 5. **Cost Optimization for All-SMD**
- **Per-unit costs** (100-unit run estimate):
  - PCB: $1.50–2.50 depending on size and layers
  - Assembly: $0.08–0.15 per board + $25–50 setup fee
  - Components (LCSC): $0.80–1.50 per board (depends on BOM)
  - **Total**: $2.50–4.00 per assembled board (landed)

- **Cost-reduction strategies**:
  - Minimize part count (remove unnecessary resistors/caps)
  - Use common passive values to reduce logistics cost
  - Batch orders: 100–500 unit runs offer better per-unit pricing

### 6. **Design Documentation**
- **Silkscreen labels** (SMD-optimized):
  - Component reference labels (R1, C1, U1) on non-placement side
  - Polarity indicators for ICs, diodes, capacitors
  - PCB edge labels: Battery+/−, Heater+/−, TC+/−
  - **Avoid clutter** on SMD side (reserved for pick-and-place machine)

- **Test points & pads**:
  - UART Pogo pads (2.54 mm pitch, labeled TX/RX/GND)
  - Power test pads (VCC, GND, VBAT, 3.3V)
  - Optional: Temperature sensor test pad (near heater)

## Migration Checklist — All-SMD Design

- [ ] **Switch to EasyEDA** or set up KiCAD with Seeed SMD library
- [ ] **Replace XIAO socket with direct ESP32-C6 SMD** (BGA or edge-connector)
- [ ] **Integrate SSD1306 OLED IC directly** (no breakout module)
- [ ] **Integrate MCP9600 thermocouple ADC IC directly** (no breakout module)
- [ ] **Replace all through-hole components with SMD equivalents**:
  - [ ] MOSFET: TO-220 → SO-8 / DFN-8
  - [ ] Diodes: axial → SOD-123
  - [ ] Resistors/caps: all → 0603 or 0805
- [ ] **Replace screw terminals & pin headers with SMD connectors or pads**:
  - [ ] Battery: 5.08mm terminal → JST-GH 1.25mm SMD
  - [ ] Heater: screw terminal → solder pads on PCB edge
  - [ ] UART: pin header → Pogo test pads
- [ ] **Design DFM (Design for Manufacturing) check**:
  - [ ] Verify all footprints in Seeed library or standard KiCAD SMD library
  - [ ] Run PCB DRC against Seeed Fusion design rules
  - [ ] Check solder mask clearance (min 0.1 mm)
  - [ ] Verify pad sizes for reflow (no undersized pads)
- [ ] **Generate final BOM**:
  - [ ] Export from KiCAD with LCSC part codes
  - [ ] Verify all parts in-stock on LCSC
  - [ ] Check lead times (aim for < 2 weeks)
  - [ ] Estimate total component cost
- [ ] **Get Fusion PCB quote**:
  - [ ] Upload KiCAD files to https://fusion.seeedstudio.com/
  - [ ] Enable PCBA (PCB Assembly) option
  - [ ] Upload BOM
  - [ ] Review quote (assembly fee + component cost)
  - [ ] Accept quote and place order
- [ ] **Update documentation**:
  - [ ] Update `PCB-PROTOTYPING.md` for SMD-only design
  - [ ] Remove socket/header references from `battery-pack.md`
  - [ ] Create `SMD-DESIGN-GUIDE.md` with footprint specifications
  - [ ] Update README.md links to SMD-focused resources

## Resources & Links

- **Seeed Studio Fusion**: https://fusion.seeedstudio.com/
- **LCSC Electronics**: https://lcsc.com/
- **KiCAD Lib LCSC (GitHub)**: https://github.com/SeeedStudio/KiCad_Lib_LCSC
- **Seeed Community**: https://community.seeedstudio.com/
- **Design Rules for Fusion**: https://wiki.seeedstudio.com/Fusion_PCB_Capability/
- **EasyEDA Tutorial**: https://docs.easyeda.com/

## Contact & Support

- Seeed Studio Forum: https://community.seeedstudio.com/
- LCSC Support: https://lcsc.com/
- ESPAtomizer project: See main README.md for contact info

---

**Last updated**: 2026-06-17  
**Status**: Active migration to Seeed ecosystem  
**Next phase**: Full BOM generation and first Fusion PCB quote
