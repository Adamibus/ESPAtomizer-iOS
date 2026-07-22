# All-SMD PCB Design for ESPAtomizer (SMD ESP32-C6 + SMD MOSFET + Integrated OLED + Integrated MCP9600)

**Approach**: Professional manufacturing via **Seeed Studio Fusion PCBA** with 100% surface-mount components and direct PCB IC integration (no breakout modules or socket headers).

## Electrical blocks to include

- ESP32-C6 controller header (XIAO module or a 2x7 socket)
- Low-side N-MOSFET power stage
  - Gate resistor: 100 Ω (series)
  - Gate pulldown: 100 kΩ to GND
  - Optional: TVS on gate if harsh environment
  - Flyback diode across inductive loads (1N5819/SS14)
  - Big copper for heater current (see widths below)
- I2C bus header (OLED + MCP9600)
  - 4-pin header: 3V3, GND, SDA, SCL
  - 4.7 kΩ pull-ups if your modules don’t include them (most do)
- Thermocouple interface (MCP9600 module) + K-type connector or solder pads
- Button to GPIO16 → GND (use onboard pull-up)
- Potentiometer to 3V3 / GND, wiper → GPIO2 (if you keep the pot)
- Power input
  - Single 1S LiPo (BAT+ / BAT− pads on XIAO) to power the whole device
  - Optional USB: allowed if your board revision supports charge‑path management (verify datasheet)
  - Common ground point (star ground) joining ESP GND and load return
  - Hardware battery disconnect switch (SPST) inline with BAT+ to cut all power when OFF (economical safety)
  - Optional reverse‑polarity protection (series Schottky or ideal‑diode controller)
  - Baseboard battery connector (J_BAT): 2‑pin 5.08 mm terminal feeding BAT+ / BAT−. Tie to the XIAO’s BAT pads via:
    - Simple jumper pads near the sockets (solder short wires from the XIAO BAT pads to TBAT+ / TBAT−)
    - Or optional pogo pins aligned to the XIAO BAT pads (requires careful mechanical alignment)

## Layout tips (critical for 3–4 A heater)

- Use short, thick traces for the high-current loop:
  - LiPo + → Heater + → Heater − → MOSFET Drain → MOSFET Source → LiPo −
  - Keep this loop off solderless breadboards; use perfboard with heavy wire jumpers or a PCB with wide copper pours.
- Trace widths (1 oz copper, ~10°C rise, conservative):
  - 4 A: 2–3 mm (80–120 mil) on outer layer; more if long
  - Or pour a solid copper plane/polygon for the heater path
- Grounding:
  - Star ground: connect ESP GND and heater return at one sturdy point
  - Keep the small-signal ground for MCP9600 and OLED away from the heavy current path
- Decoupling:
  - Place 0.1 µF near ESP 3V3 pin; 10 µF bulk near module input
  - Place 0.1 µF near MCP9600 VCC
- Thermocouple routing:
  - Twist TC leads; avoid parallel routing next to the heater traces
  - Keep TC input away from MOSFET drain node and switching edges

## Connectors and footprints

**⚠️ All footprints and components should be sourced from [Seeed Studio Fusion library](../SEEED-STUDIO-FUSION-INTEGRATION.md) for manufacturing compatibility.**

### Component Specifications

- **Heater and supply**: 2-pin screw terminal (5.08 mm) — LCSC sourced, or XT30/XT60 pigtail for LiPo (Seeed-branded preferred)
- **Thermocouple**: mini K-type socket or 2-pin terminal with clear polarity — standard components available via LCSC
- **I2C headers**: 4-pin 2.54 mm female socket header (3V3/GND/SDA/SCL) — keyed or labeled
  - KiCAD footprint: `Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical` (Seeed verified)
  - LCSC alternatives: Multiple verified pin socket sources
- **MOSFET**: TO-220 (e.g., IRLB8721/IRLZ44N) or logic-level power DFN/SO-8 (if SMD) — LCSC sourced
- **Button**: 6×6 mm tact or panel-mount — standard KiCAD library
- **OLED module**: SSD1306 I2C breakout with 1×4 female socket header
  - KiCAD footprint: `Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical`
  - LCSC sourced modules available
- **Battery connector**: JST-PH 2-pin (J_BAT) labeled BAT+/BAT−, or upgrade to Seeed XT30 for robust builds
  - KiCAD footprint: Search LCSC for JST-PH footprint + connector
  - Add SW1 (SPST) on BAT+ for full disconnect

### Socket-mount all modules (cost-saving, serviceable)

**⚠️ Use [Seeed Studio Fusion Library](https://github.com/SeeedStudio/KiCad_Lib_LCSC) footprints for best manufacturing compatibility.**

Per your requirement, mount the XIAO ESP32‑C6, the OLED, and the K‑type/MCP9600 board in sockets (female pin headers) so they can be plugged in and replaced:

- XIAO ESP32‑C6: two rows of 1×7 female pin sockets, 2.54 mm pitch
  - Center‑to‑center row spacing: use ~15.5 mm for this ESP32‑C6 board (per measured update). Verify on your exact module before placement.
  - KiCad footprints (Seeed verified): 
    - From standard library: `Connector_PinSocket_2.54mm:PinSocket_1x07_P2.54mm_Vertical` (x2)
    - From LCSC/Seeed: Check for verified pin socket part numbers in Seeed library
  - Leave keep‑out under the module for components; don’t place tall parts between the rows.

- OLED (SSD1306 I2C module): one 1×4 female pin socket, 2.54 mm pitch
  - Label pins on the PCB: 3V3, GND, SDA, SCL (left→right). Confirm your module’s header order; many use GND, VCC, SCL, SDA or VCC, GND, SCL, SDA.
  - KiCad footprint (Seeed verified): 
    - From standard library: `Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical`
    - From LCSC/Seeed: Multiple verified sources available

- MCP9600 thermocouple breakout: one 1×7 female pin socket, 2.54 mm pitch
  - Route at minimum: 3V3, GND, SDA, SCL. Optional: ALERT, A0, A1 to allow address/alert features.
  - If you want only 4 pins, you can use a 1×4 socket and strap A0/A1 on the board; using a 1×7 keeps it flexible.
  - KiCad footprint (Seeed verified): 
    - From standard library: `Connector_PinSocket_2.54mm:PinSocket_1x07_P2.54mm_Vertical`
    - From LCSC/Seeed: Check for verified pin socket part numbers in Seeed library

Recommended socket height: standard "Arduino‑style" 8.5–9 mm tall female sockets work well with common breakouts. Low‑profile sockets are also fine if mechanical clearance allows. All major socket styles available via LCSC.

## Schematic essentials (net list sketch)

- GPIO1 → 100 Ω → MOSFET Gate; Gate → 100 kΩ → GND
- MOSFET Drain ↔ Heater −; Heater + ↔ +V (VBUS or LiPo +)
- MOSFET Source ↔ GND (common with ESP)
 - Battery (J_BAT): BAT+ → SW1 → TBAT+; BAT− → TBAT− (jumper or pogo to XIAO BAT pads)
- MCP9600: VCC=3.3 V, GND, SDA=GPIO22, SCL=GPIO23 (I2C @0x60)
- OLED: VCC=3.3 V, GND, SDA=GPIO22, SCL=GPIO23 (I2C @0x3C)
- Button: GPIO16 ↔ switch ↔ GND
- Pot (optional): 3.3 V ↔ POT ↔ GND; wiper → GPIO2

## From perfboard to PCB

**Recommended workflow**: Use **Seeed Studio Fusion Service** for PCB + component assembly. See [Seeed Studio Fusion Integration Guide](SEEED-STUDIO-FUSION-INTEGRATION.md) for detailed instructions.

- **Design tools**: 
  - **EasyEDA** (recommended): https://easyeda.com/ — Native Seeed Fusion integration, auto-links LCSC components
  - **KiCAD** (with Seeed library): https://kicad.org/ + [Seeed KiCAD_Lib_LCSC](https://github.com/SeeedStudio/KiCad_Lib_LCSC)
- **PCB Manufacturing**:
  - Make a 2-layer PCB with:
    - Top: signals + I2C, bottom: ground plane and heater return (or vice versa)
    - Wide copper for heater path; vias stitching if you split across layers
    - Mounting holes and a clear airflow path if you add a heatsink
  - Silkscreen labels: +V, GND, SDA, SCL, OUT, HEATER ±, TC T+/T−
  - Test points: GATE, DRAIN, SOURCE, 3V3, VBAT, SDA/SCL
  - **Seeed Design rules**: Verify design against [Seeed Fusion PCB capability specs](https://wiki.seeedstudio.com/Fusion_PCB_Capability/)

## Bring-up checklist

- With heater disconnected: power from USB; confirm OLED + MCP9600 appear; GET prints temperatures
- Connect LiPo; verify hardware battery switch SW1 toggles power and polarity is correct
- Manual full power test (Serial): `O 1023`; measure Vload and Vgs
- Monitor MOSFET temperature; if warm, consider a lower Rds_on device or larger copper/heatsink

## Suggested MOSFETs for 3.3 V gate

- Through-hole: IRLB8721 (better at low Vgs than IRLZ44N), IRLZ44N (works, older)
- SMD: AO3400A (OK up to ~2 A continuous with thermal care), modern power FETs with ≤15 mΩ @ 2.5–3 V preferred

## Safety

 - Never short the pack; always use the hardware battery switch to turn the device fully OFF when not in use
- Keep high-current paths short; secure wiring to avoid hot spots
- Use ceramic adhesive/insulation for the sensor; avoid exposed conductive parts near the coil
 - Pogo option: If you pick up the XIAO BAT pads with pogo pins, ensure consistent compression and retention so contact cannot intermittently open under vibration.
