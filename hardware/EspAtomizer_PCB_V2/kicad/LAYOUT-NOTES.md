# ESPAtomizer v2 — Layout Notes (ESP32-C6-MINI-1-N4 specific)

This file contains practical layout guidance for placing the `ESP32-C6-MINI-1-N4` module and the power/sensor circuitry (MAX6675, MCP73871 charger, DW01 protector, USB‑C, MOSFET). Treat this as a checklist — verify module mechanical drawings and datasheets before finalizing footprints.

1) Module footprint verification
- Do not use a best-effort footprint for production. Replace placeholder with the manufacturer's recommended land pattern for `ESP32-C6-MINI-1-N4`.
- Confirm pad numbering, pad shape (castellated length/width), recommended soldermask openings, and recommended courtyard/keepout.

2) Antenna keepout and RF clearance
- Respect the antenna keepout zone on the datasheet: keep copper, ground pours, tall components, and metal shielding at the recommended distance from the antenna.
- Do not place ground pours directly over the antenna. If the module requires a ground plane under the module, follow the datasheet's guidance precisely.

3) Decoupling and power placement
- Place 100 nF ceramic + 10 µF (or specified decoupling) directly adjacent to the module VCC pins (0.5–2 mm max trace). One decoupling per VCC pin group if module has multiple VCC pads.
- Place regulator output caps (LDO/buck) close to the regulator pins; route VIN from SYS using wide traces for current.

4) Battery & Charger layout
- Place MCP73871 close to the USB‑C connector/VIN entry point. Place input fuse and TVS on VIN as the first devices from VBUS.
- Place BAT node and battery connector close together; place DW01/FS8205A protector between BAT and SYS with short, wide traces.
- Program resistor (Rprog) for MCP73871 should be placed close to the chip.

5) USB‑C and connectors
- Choose a USB‑C footprint that matches your mechanical needs (side‑entry vs top‑entry). Confirm its anchor/mounting pads and ensure mechanical stability.
- Route CC1/CC2 to resistors as close to the connector as possible. For device-only designs, use Rd pull-down resistors (e.g., 5.1kΩ) per spec.

6) MAX6675 placement and routing
- Place MAX6675 close to the thermocouple wiring entrance point; keep the SPI traces (SCK, CS, SO) short and direct to the module.
- Add 0.1µF and 10µF decoupling at MAX6675 VCC near its VCC pin.

7) MOSFET and heater routing
- Place heater MOSFET close to the heater connector so high current flows on short, wide traces only.
- Use large copper pour or multiple traces for heater current; add thermal vias if routing current to inner or bottom layers.
- Place gate resistor (100Ω) and pulldown (100k) adjacent to the MOSFET gate pad.

8) Thermal considerations
- For buck regulators or MOSFETs that dissipate heat, expose a large copper area on the top and bottom layers and use thermal vias to spread heat.
- Keep high‑power components off the module antenna zone.

9) Grounding and planes
- Use a solid ground plane where possible; split ground planes only with a good reason. Add stitching vias around the module and high-current areas as needed.

10) Test points and programming
- Add a 4-pin programming header (GND, TX, RX, EN/RESET) or route to a small 2x3 header for convenient flashing and serial log access.
- Add test points for BAT, SYS, 3V3, and MOSFET drain for bench debugging.

11) DRC checks and manufacture readiness
- Before fab, run DRC with the chosen fab's rules: minimum annular ring, soldermask slivers, minimum soldermask clearance for castellated pads, and pad-to-pad spacing.
- Confirm that castellated pads meet the board house's capability for plating and edge milling.

12) Final verification
- Cross-check every footprint pad pitch and dimension against the vendor datasheet (module, USB‑C, JST battery, MCP73871 SOT-23-5, MAX6675 SOIC‑8, MOSFET package).
- Perform an ERC on schematic (fix any strap‑pin warnings) and DRC on PCB prior to ordering prototypes.

Quick checklist (before sending to fab):
- Module mechanical PDF on hand — YES/NO
- All critical footprints verified against datasheets — YES/NO
- Antenna keepout respected — YES/NO
- High-current traces width checked & thermal vias added — YES/NO
- Programming header & test points included — YES/NO
- DRC clean for chosen fab rules — YES/NO

If you'd like, I can now:
- a) Read your `ESPAtomizer_v2.kicad_sch` (read-only) and produce a strap-pin conflict report; or
- b) If you upload the official `ESP32-C6-MINI-1-N4` mechanical datasheet, I will convert it into an exact KiCad footprint and replace the placeholder in `hardware/ESPAtomizer_PCB_v2/kicad/footprints/ESPAtomizer.pretty/` (marked with verification notes).
