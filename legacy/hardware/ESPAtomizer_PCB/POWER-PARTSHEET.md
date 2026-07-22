# Partsheet — Compact 1S Power-Path Revision (MCP73871 + DW01/FS8205A)

Date: 2025-11-26
Target: new revision of `hardware/ESPAtomizer_PCB` (do NOT modify the existing KiCad project files)

Scope
- Provide recommended, compact part selections for a 1S Li‑ion power-path design using a power-path charger (MCP73871), battery protection (DW01 + FS8205A), USB‑C receptacle with CC handling for default 5 V, a 3.3 V system regulator (buck or LDO), onboard MAX6675 thermocouple interface, and recommendations for an SMD ESP32 module.
- Include suggested footprints and KiCad symbol/footprint search terms so you can locate or create matching symbols in your KiCad libraries.

Important notes before using this partsheet
- Validate footprint geometry against vendor mechanical drawings before PCB layout; some vendors use slightly different land patterns for the same package name.
- Where a single-package solution exists (protector + FET), you may substitute. The DW01 + FS8205A combination is extremely common for 1S protection and easy to source.
- I provide suggested KiCad footprint names as search terms (they are *suggestions*). Confirm exact symbol/footprint names in your KiCad library or from third‑party KiCad libs (e.g., digikey footprints, SnapEDA, official KiCad libs).

Table: recommended components (starter)
- Columns: `Ref` | `Qty` | `Role` | `Recommended Example Part (search)` | `Package / Footprint (suggested)` | `KiCad footprint / symbol (search term)` | `Notes`

Ref: U_MODULE
- Qty: 1
- Role: SMD Wi‑Fi/BT/802.11/ESP32 module (use module with integrated antenna)
- Recommended Example Part (search): Espressif "ESP32‑C6 MINI" module family (vendor variant with integrated antenna) or similar SMD module for ESP32‑C6
- Package / Footprint: module (dimensions vary by vendor) — check vendor mechanical
- KiCad footprint / symbol (search): `espressif`, `ESP32-C6-MINI`, `ESP32-MODULE` (verify)
- Notes: using a module avoids RF-matching work. Choose a module with 3.3 V Vcc input or one that contains its own regulator depending on your architecture.

Ref: U_CHG (Charger + power-path)
- Qty: 1
- Role: 1S Li‑ion charger with power-path (device usable while charging)
- Recommended Example Part (search): `MCP73871` (Microchip) — power-path charger IC for single‑cell Li‑ion
- Package / Footprint: SOT‑23‑5 (check exact variant e.g., `MCP73871T-2ACI/OT`) or DFN per datasheet
- KiCad footprint / symbol (search): `MCP73871`, `SOT-23-5` or `SOT23-5` symbol
- Notes: MCP73871 provides automatic power-path switching and USB current limit options; place close to USB input.

Ref: U_PROT (Battery protector) — two components commonly used together
- Qty: 1 each (DW01 + FS8205A)
- Role: Battery overcharge/overdischarge + dual MOSFET protect
- Recommended Example Part (search): `DW01` (protection IC) + `FS8205A` (dual MOSFET)
- Package / Footprint: DW01: SOT‑23‑6 (or SOT‑23‑5 variants — check vendor); FS8205A: SOP‑8/WSON depending on vendor
- KiCad footprint / symbol (search): `DW01`, `FS8205A`, `SOP-8` or `SOT-23-6`
- Notes: Common textbook 1S protection combo used on many TP4056 modules. Ensure thermal and copper have large enough areas for MOSFETs.

Ref: CON_USB
- Qty: 1
- Role: USB‑C receptacle (5V input)
- Recommended Example Part (search): USB‑C receptacle, side‑entry, 5-pad (VBUS, CC1, CC2, GND, SBU optional). Example mechanical family: Amphenol or TE USB‑C micro‑receptacle.
- Package / Footprint: USB‑C Receptacle SMD (verify mechanical drawing)
- KiCad footprint / symbol (search): `USB_C_Recept`, `USB-C_Receptacle` or library `Connector_USB:USB_C_Receptacle`
- Notes: For basic 5 V operation (no PD), use two Rd resistors; for PD, add PD controller.

Ref: R_CC1, R_CC2
- Qty: 2
- Role: CC pull resistors to advertise as device (UFP)
- Recommended: 5.1 kΩ 0402 or 0603 (Rd value typical for default) — see USB-C Rd recommended values
- KiCad footprint: `Resistor_SMD:R_0402` or `R_0603`
- Notes: Use two identical Rd resistors to CC1 and CC2 to claim default current draw (host supplies 5V). If you want to indicate higher currents or support PD, use a PD controller instead.

Ref: F1
- Qty: 1
- Role: Input fuse / PTC for VBUS
- Recommended: 500 mA — 2 A polyfuse SMD (e.g., 1206 PTC) depending on desired current
- Footprint: `PTC_1206` or `PTC_0805`

Ref: D_VBUS_TVS
- Qty: 1
- Role: Transient suppression on VBUS
- Recommended: `SMBJ5.0A` or similar SMBJ TVS (5.0 V standoff) SMD
- Footprint: `SMBJ`

Ref: U_REG (3.3V regulator)
- Qty: 1
- Role: system 3.3 V regulator if module requires external 3.3V
- Recommended (buck for efficiency): `MP2307DN` or `MP2307DN-LF` (SOT‑23‑6 buck) — example step‑down for higher current
- Alternate (LDO, small, low noise): `MCP1700-33` (SOT‑23‑3 LDO) for very small boards with low current
- KiCad footprint / search: `MP2307`, `SOT-23-6`, `MCP1700-33`
- Notes: If the chosen ESP module accepts BAT directly (contains regulator), you may not need a system regulator. Prefer a buck converter if you expect >200–300 mA continuous draw.

Ref: U_TC (MAX6675)
- Qty: 1
- Role: Thermocouple-to-digital IC (K-type)
- Recommended example part: `MAX6675` (SOIC‑8) — simplest option
- Package / Footprint: SOIC‑8 (3.9×4.9 mm typical) or SOIC‑8_208mil
- KiCad footprint / symbol (search): `MAX6675`, `SOIC-8_3.9x4.9mm`
- Notes: Place this IC close to thermocouple input and decouple VCC with 0.1 µF and 1 µF.

Ref: Q_MOSFET (heater MOSFET)
- Qty: 1
- Role: Heater switch MOSFET
- Requirements: logic‑level N‑MOSFET with guaranteed Rds(on) at VGS = 3.3 V (look for Rds(on) ≤ 50 mΩ ideally), low thermal resistance package (e.g., SOT‑223 for higher power or SMD DPAK/SO‑8 for heatsinking)
- Example search terms: `60V 3.3V logic MOSFET Rds(on) 10-50 mOhm` or families: `IRLZ44` (through‑hole example), `SI2302` (SMD small FETs — verify Rds(on) at 3.3 V)
- KiCad footprint: `SOT-223`, `DPAK`, `SO-8` depending on chosen MOSFET
- Notes: Place R3 (100 Ω series) and R2 (100 kΩ pulldown) at the MOSFET pins; move gate net off of any strapping pin (recommend GPIO16).

Ref: Passive & decoupling
- C_bulk: 10 µF–22 µF, 0805/1206 low-ESR ceramic or electrolytic near charger/regulator
- C_decouple: 0.1 µF ceramic 0402 or 0603 close to each IC VCC pin
- R_series for SCK: optional 10–47 Ω on SPI lines
- Resistors: standard 0402 or 0603 footprints

Footprint/KiCad symbol guidance
- Use official KiCad libs where possible for generic footprints: `Connector_USB:USB_C_Receptacle`, `Capacitor_SMD:C_0603`, `Resistor_SMD:R_0402`, `Package_SO:SOIC-8_3.9x4.9mm`
- For specialized ICs (MCP73871, MP2307, MAX6675, DW01, FS8205A), search the KiCad library browser or import footprints from vendor STEP/land patterns and create symbols if needed.
- If you use 3rd-party footprints (manufacturer), add a `vendor` folder to the project and include mechanical drawings in the repo.

Schematic connectivity notes (high level)
- USB‑C VBUS → F1 (fuse) → D_VBUS_TVS → VIN of charger / MCP73871 input
- CC1/CC2 → Rd (5.1k) → GND (for device-only default 5V)
- Charger `SYS` / `VOUT` node → battery protector network and battery node (BAT)
- Charger `SYS` → system regulator (buck or LDO) if module expects 3.3V
- Battery protector between BAT connector and battery / charger node (DW01 + FS8205A)
- BAT_ADC → voltage divider to scale BAT voltage to ADC range; add TVS/clamp to protect ADC pin
- MAX6675: VCC → 3.3V, GND → GND; SPI pins (SCK/CS/SO) to mapped GPIOs; decouple with 0.1 µF at VCC

Layout & thermal guidance
- Place charger IC and USB connector at board edge; place input fuse and TVS close to connector.
- Place MOSFETs (FS8205A) with exposed pad or thermal vias and copper pour for heat spreading.
- Place 0.1 µF decoupling as close as possible to IC VCC pins (2–4 mm max).
- Keep thermocouple traces short, routed away from heater traces; use ground guard if possible.

Verification checklist before sending for fabrication
- Confirm footprints against manufacturer mechanical datasheets.
- ERC/DRC in KiCad with the new symbol/footprint libraries.
- Verify charger thermal dissipation and pad copper for heat spreading.
- Check battery protection polarity and orientation.
- Validate antenna keepout area of chosen SMD module.

Optional: exact part picks I can obtain for you
- If you want, I can produce a fully exact BOM with manufacturer part numbers and precise KiCad footprints (I will look them up from distributor datasheets). Tell me and I will fetch and compile the exact PNs and footprint files.

Next steps
- Confirm you want the MCP73871 power-path approach (recommended) OR the TP4056 + protector (cheaper but no power-path).
- If MCP73871 chosen, I will prepare a schematic block (KiCad symbol + footprint suggestions) for the charger + DW01/FS8205A protector and a small layout guideline PDF you can drop into a new KiCad project.


---

If you want me to proceed with the exact BOM (manufacturer P/N + footprint) and KiCad symbol/footprint attachments, say "Produce exact BOM" and I'll fetch datasheets and create the parts with precise footprints.