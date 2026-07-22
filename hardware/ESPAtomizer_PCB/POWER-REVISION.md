# Power Revision & USB-C Charging — Recommendations and PCB Overhaul

Date: 2025-11-26
Target: `hardware/ESPAtomizer_PCB` (do NOT modify the existing KiCad project files in-place)

Purpose
- Provide clear recommendations and step-by-step PCB/schematic overhaul instructions to add USB-C charging (device), support for 1S (mandatory) and optional 2S Li pack, allow the board to be usable while charging (power-path), and consider moving from the XIAO socket module to an SMD ESP32 module or SMD chip.

High-level choices (pick one path)
- Option A — Minimal / compact (recommended for first revision):
  - Support 1S Li-ion (single cell). Use USB-C receptacle as 5V source with CC resistors (UFP device). Use a power-path charger IC (e.g., MCP73871 family or TI BQ24072) so the board can run while charging.
  - Use an SMD ESP32 *module* (SMD module with integrated antenna & RF matching) rather than bare ESP32 chip to avoid RF layout complexity.
  - Replace the MAX6675 socket with onboard MAX6675 (SOIC-8) and keep thermocouple pads.

- Option B — Flexible, higher capability (if you need 2S support):
  - Support either 1S or 2S by designing a swappable power stage or using a multi-cell charger (e.g., dedicated 2S charger/IC with balance). This is more complex: different charger ICs, battery protection, and layout. Consider limiting to 1S initially.

Key design goals and constraints
- Device must be usable while charging (no manual battery disconnect required).
- Prefer SMD parts and compact footprint for small PCB.
- Keep thermocouple traces isolated from heater and power traces to preserve measurement accuracy.
- Avoid complex RF matching by using an SMD module with integrated antenna unless you want a full-radio design.

Recommended component choices (starter list)
- USB-C connector: `USB-C receptacle (side-entry, 4-lane optional)` supporting VBUS, CC1, CC2, GND, SBU optional.
- CC resistors for device-only (no PD): two Rd resistors — typically 5.1 kΩ on CC1 and CC2 for UFP to indicate device/default power (host supplies 5V). If you plan to support higher currents or PD, use a USB-PD controller (STUSB4500).
- Charger (1S, power-path recommended):
  - MCP73871 (Microchip) — power‑path charge management with USB current limiting. Small SOT‑23 packages and good for powering device while charging.
  - Alternative: TI `BQ24072` / `BQ24090` family — power-path + charging and thermal regulation; choose based on availability.
- Battery protection: DW01 + FS8205A dual FET for 1S protection, or use integrated protector IC with appropriate dual-MOSFET.
- Charger alternative (smallest / cheapest): TP4056 module (linear) + separate protection IC (DW01/FS8205A). Note: TP4056 is linear and will dissipate heat; it's not power-path — the board may reset when current is high while charging.
- 3.3V regulator for ESP & peripherals (if module needs 3.3V input):
  - If the SMD ESP32 module requires 3.3V rail, use a buck converter (recommended) or LDO depending on current: `MP2307` (small buck, ~3A) or `MCP1700-33` LDO (if low current and heat acceptable).
- Battery pack options:
  - 1S Li-ion (3.7 V nominal) — simplest and recommended.
  - 2S Li-ion — requires a dedicated 2S charger / balancer or multi-cell charging IC (more complex) and a different protection approach.

USB-C choices and CC handling
- If you only need to run from a standard USB host or charger at 5V (no PD):
  - Implement the USB-C receptacle with CC1 and CC2 each pulled to ground through Rd (5.1 kΩ) to advertise as UFP/Device. The host will put 5V on VBUS.
  - Add an input fuse (PTC or 500 mA–2 A polyfuse) and an ideal‑diode or power switch (e.g., `TPS2546` if you need USB current negotiation or `STUSB1602` for PD).
- If you want PD/negotiation or higher currents: add a PD controller like `STUSB4500` or `FUSB302` + appropriate charger that supports PD.

Power-Path and charging while running
- Use a charger IC with integrated power-path (MCP73871, BQ24072, etc.). That lets the board source ESP & peripherals directly from VBUS when available while charging the battery safely.
- Add an ideal‑diode or reverse current protection between charger and system rail if your charger IC doesn't include power-path.

1S vs 2S considerations
- 1S (single cell): common, many small ICs available. Use DW01 + FS8205A for protection; charger: MCP73871 or TP4056 (no power-path).
- 2S: must use a 2-cell charger with balance and appropriate protection (e.g., BQ25895 family or dedicated multi-cell chargers). Also battery-protection topology changes (multi-cell protection ICs and balancing circuitry). Designing for both in one PCB is possible but increases complexity and board area.

SMD ESP32 vs XIAO module
- Recommendation: use an SMD ESP32 *module* (SMD module with integrated crystal, flash, and antenna) rather than the bare SMD Wi‑Fi chip unless you are comfortable with RF matching, antenna design, and certification. Modules reduce RF risk and greatly speed development.
- If you choose to use a bare SMD ESP32 chip (not a module):
  - You must design the RF matching network, antenna, microstrip traces, keepout areas, test points, and perform RF testing. This increases time and risk.

Detailed PCB overhaul instructions (step-by-step)
1) Constraints & preparation
   - Branch a new PCB project (do not modify the existing `ESPAtomizer.kicad_pcb` files).
   - Prepare target constraints: board outline, connector placement (USB-C near board edge), battery connector position (JST near edge), and thermal relief for charger IC.

2) Power input stage (USB-C)
   - Place the USB-C receptacle on board edge with VBUS, GND, CC1, CC2 accessible.
   - Add two CC Rd resistors (5.1 kΩ) from CC1 and CC2 to GND if you only need default 5 V.
   - Add an input fuse or PTC (e.g., 500 mA–2 A depending on expected charge current) on VBUS.
   - Add an input TVS (SMBJ5.0A or similar) for surge protection on VBUS.

3) Charger & protection
   - Place chosen charger IC (MCP73871 recommended for 1S + power-path) close to USB connector to minimize high-current path length.
   - Place battery protection (DW01 + FS8205A) between battery connector and charger/battery node.
   - If using TP4056, remember it is linear and place it with good thermal copper pour.
   - Provide programming pads for I2C/SWD and test points for VBUS, BAT, SYS (3.3V), GND.

4) System power rail
   - If ESP module expects 3.3V: add a buck converter (if high current) or LDO (if low current). Place decoupling capacitors close to module power input pads (0.1 µF + 4.7 µF recommended; refer to regulator datasheet).
   - Provide power-path components to feed `3V3` rail when VBUS present; ensure OR-ing or power-path IC handles seamless switchover.

5) Battery support options
   - For 1S: layout DW01 + FS8205A footprint with proper thermal relief and keep sense traces short.
   - For optional 2S: if including 2S, choose a 2S charger IC and add balancing resistors, and ensure board footprint and BOM reflect this complexity.

6) MAX6675 on-board
   - Replace socket with SOIC‑8 footprint for MAX6675; place it near thermocouple connector/pads.
   - Add 0.1 µF and 1 µF caps close to VCC of MAX6675.
   - Route MAX_SCK, MAX_CS, MAX_SO to the ESP SMD module pins with short traces. Add optional 10–47 Ω series resistors on SCK and SO if signal integrity requires.
   - Add a 0-ohm resistor or solder jumper to allow bypassing the onboard MAX6675 (useful for keeping the option of an external board or rework).

7) MOSFET & heater
   - Keep MOSFET and heater traces wide and thermally coupled to copper pours for heat spreading.
   - Place gate series resistor and gate pulldown right at MOSFET pad. Ensure the MOSFET's gate net is not on a strapping/boot pin — move to a non-strap GPIO (we recommend GPIO16) and update `config.h` accordingly.

8) RF / antenna
   - If you use an SMD module: place module according to vendor placement notes and keep its antenna keepout area clear. Ensure no copper or large pours under antenna area.
   - Add an RF test pad if the module supports it.

9) Decoupling & layout specifics
   - Place 0.1 µF and 1 µF ceramic caps within 2–4 mm of the SMD module `3V3` pad(s). Add a bulk 10–22 µF near charger/regulator.
   - Use star routing for battery return: connect battery negative directly to the main ground plane; ensure heater return and sensitive analog grounds are routed carefully.
   - Add test points: VBUS, BAT, SYS (3.3V), GND, I2C, SPI, MOSFET_Gate, MOSFET_Drain.

10) Silkscreen / documentation
   - Update silkscreen to label USB-C orientation, battery connector polarity, and thermocouple polarity.
   - Document in BOM recommended footprints and hand-solder rework notes.

Bring-up and test plan
- Stage 1: Validate power rails without battery installed
  - With USB-C plugged into a limited-power supply (current-limited bench supply), verify VBUS presence, charger IC behavior, and SYS rail (3.3V) regulation.
- Stage 2: Validate charging behavior
  - With a known-good 1S Li cell, verify charger enters charge state, current limit works, and battery protection engages if you short or reverse polarity.
- Stage 3: Functional test with ESP module
  - Boot the SMD module and run minimal firmware to check ADC readings (BAT_ADC), MAX6675 measurements, and heater MOSFET control while plugged into USB.
- Stage 4: Thermal & long-duration tests
  - Monitor charger, regulator, and MOSFET temperatures while charging and heating loads are present.

BOM & footprints (example recommendations)
- USB-C: `USB-C receptacle, 5 pins minimum, side-entry` (pick mechanical footprint to suit enclosure)
- CC resistors: `5.1 kΩ 0402` × 2
- Charger: `MCP73871`, SOT23-5 or recommended package
- Battery protector: `DW01 + FS8205A` or single-package protector IC
- 3.3V regulator: `MP2307DN` (buck) or `MCP1700-33` (LDO) depending on current requirements
- Thermocouple IC: `MAX6675` (SOIC-8) or `MAX31856` if you want more capability
- MOSFET: logic-level N‑MOSFET with Rds(on) guaranteed at VGS=3.3 V (e.g., `Si2302` alternatives — verify in datasheet)

Tradeoffs & notes
- TP4056 is cheap and tiny but linear (heat) and lacks power-path — not recommended if you must run heavy loads while charging.
- MCP73871 (or TI equivalents) provide power-path and better UX when device is used while charging.
- Supporting 2S increases complexity; if you plan both 1S and 2S, consider two separate BOM variants or a jumper-configurable approach.
- Using a full bare SMD ESP32 chip (no module) increases RF and certification complexity; prefer a tested SMD module for faster development.

Next steps I can take (pick one)
- I can produce a detailed part-selection sheet with exact part numbers, footprints, and KiCad symbol/footprint suggestions for Option A (1S + MCP73871 + MAX6675 SOIC), or
- I can produce a schematic-level PDF/PNG block to paste into a new KiCad project for the power stage and on-board MAX6675, or
- I can prototype a small reference board (separate repo or new KiCad project) that demonstrates USB-C charging + MCP73871 power-path + SMD ESP32 module and MAX6675.

Which next step do you prefer? If you want me to proceed, tell me whether you prefer the "compact 1S power-path" route (MCP73871) or the "cheap TP4056 + protector" route, and whether you want USB-C PD support (higher current) or only basic 5V USB-C.
