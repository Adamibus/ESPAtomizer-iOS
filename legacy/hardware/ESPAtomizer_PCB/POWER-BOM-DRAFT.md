# POWER BOM (Draft) — MCP73871 1S Power-Path Approach

Date: 2025-11-26
Status: Draft — some distributor PDFs could not be downloaded during automated fetch; links/PNs flagged for verification.

Assumptions:
- Default: basic USB‑C 5 V device (no USB‑PD), charging current default ~1 A.
- Target battery: 1S Li‑ion (3.7 V nominal). 2S support not included in this BOM (design changes required).
- Use an SMD ESP32 module (ESP32‑C6 family module) that provides RF/antenna integration.

Notes: Verify all footprints against vendor mechanical drawings before layout. I can fetch exact distributor PN/footprints next if you want — say "Fetch exact PNs".

| Ref | Role | Manufacturer / Part (example) | Manufacturer P/N | Suggested package / footprint | KiCad search term (suggest) | Distributor / Notes |
|---|---|---:|---|---|---|---|
| U_CHG | 1S charger, power-path | Microchip MCP73871 | `MCP73871` | SOT‑23‑5 or DFN variant per datasheet | `MCP73871`, `SOT-23-5` | Microchip product page: https://www.microchip.com/en-us/product/MCP73871 |
| U_TC | Thermocouple converter | Analog Devices MAX6675 | `MAX6675KSI+` | SOIC‑8 (300 mil) | `MAX6675`, `SOIC-8_3.9x4.9mm` | Analog product page: https://www.analog.com/en/products/MAX6675.html |
| U_PROT_IC | Battery protector IC | DW01 (common) | `DW01` | SOT‑23‑6 or SOT‑23 variant (check datasheet) | `DW01` | Often sourced as DW01 or equivalent — verify distributor PN |
| U_PROT_FET | Dual MOSFET protector | FS8205A (dual FET) | `FS8205A` | SOP‑8 or equivalent (check vendor) | `FS8205A` | Common 1S protection FET; verify footprint vendor-specific |
| CON_USB | USB‑C receptacle | generic USB‑C Receptacle (side-entry) | (choose vendor e.g., Amphenol / TE) | USB‑C SMD receptacle footprint | `USB-C_Receptacle` | Pick mechanical variant for enclosure; confirm footprint
| U_REG | 3.3 V regulator (if module needs external 3.3V) | Monolithic Power Systems MP2307DN (buck) or MCP1700-33 (LDO) | `MP2307DN` / `MCP1700-33` | SOT‑23‑6 (buck) / SOT‑23‑3 (LDO) | `MP2307`, `MCP1700-33` | If ESP module has onboard regulator you may omit
| U_ESP | SMD ESP32 module | Espressif ESP32‑C6 MINImodule (example) | `ESP32-C6-MINI` (vendor-specific) | module pad array — follow vendor mechanical | `ESP32-C6` or `ESP32-MODULE` | Choose vendor module variant with integrated antenna; check pinout
| Q_HEAT | Heater MOSFET | Logic-level N‑MOSFET (3.3 V gate) | e.g., `SI4410` or other low Rds_on at 3.3V (pick) | DPAK / SO‑8 / SMD chosen | `MOSFET logic-level` | Select MOSFET with Rds(on) < ~50 mΩ at VGS=3.3V; verify thermal pad
| R_CC1 / R_CC2 | USB‑C CC pull resistors | Generic SMD resistor | 5.1 kΩ 0402 | 0402 or 0603 | `R_0402` | Two resistors to CC1 and CC2 to advertise device (Rd)
| F1 | VBUS input polyfuse/PTC | e.g., Bourns MF‑R050 | selected PTC | 1206 or 0805 | `PTC_1206` | Choose current rating (500 mA–2 A) per charge current
| D_VBUS_TVS | VBUS transient suppressor | SMBJ5.0 (TVS) | `SMBJ5.0A` | SMBJ footprint | `SMBJ` | Protect USB input
| C_decoup | Decoupling caps | 0.1 µF ceramic | 0402 / 0603 | `C_0402` | Place 0.1 µF at each IC VCC
| C_bulk | Bulk capacitor | 10–22 µF ceramic | 0805 / 1206 | `C_0805` | Place near charger/regulator
| R_GATE | Gate resistor | 100 Ω | 0603 | `R_0603` | Place at MOSFET gate (close to MOSFET)
| R_PULL | Gate pulldown | 100 kΩ | 0603 | `R_0603` | Pulldown to ground; but avoid if on strapping pin (move gate to GPIO16)
| BAT_ADC_DIV | Voltage divider | resistors to scale BAT to ADC | 0402/0603 | `R_0402` | Add input clamp (e.g., 5.1V zener or small TVS + series resistor) and 10 nF cap per docs


Caveats & verification steps required:
- I could not reliably download some PDF datasheets in this run (transient fetch errors). I included manufacturer part names and product pages where available (Microchip / Analog). I recommend verifying these PNs and downloading the vendor mechanical drawings before starting layout.
- For `DW01` and `FS8205A` find the distributor PN matching your preferred vendor (Mouser/Digikey) — footprints vary by vendor/packaging.
- For the USB‑C receptacle choose a mechanical variant that fits your enclosure; footprints differ across vendors.
- For the SMD ESP32 module, choose a vendor-approved module and download the module mechanical + pinout PDF.

Next actions I can do for you:
1. Retry fetching and download all required datasheets and mechanical drawings, then produce a verified BOM CSV with exact distributor P/Ns and attach KiCad footprints (my recommended next step).
2. Produce KiCad symbol + footprint stubs (Schematic block) for the charger + protection + USB‑C handling that you can import into a *new* KiCad project (I will not modify your existing project).

Say "Fetch exact PNs" to have me retry fetching datasheets and assemble the final verified BOM + footprints.

## Distributor mapping guide (quick)
Use this checklist to find matching DigiKey / Mouser ordering P/Ns for each manufacturer part listed in the BOM.

1. Start with the manufacturer P/N and datasheet
	- Search the manufacturer P/N (e.g., `MCP73871`) in the distributor site search box.
	- Open the official datasheet PDF from the manufacturer page and note package options (SOT-23-5, DFN, SOIC-8, etc.) and thermal/landpattern notes.

2. Filter distributor results by package and status
	- On DigiKey/Mouser filter by `Package/Case` matching the datasheet (e.g., `SOT-23-5`, `SOIC-8`).
	- Filter by `Manufacturer` (Microchip, Analog Devices, Monolithic Power, Espressif) to avoid cross-referenced parts unless you intend substitutions.
	- Prefer parts marked `Active` and with available stock.

3. Confirm electrical variant and ordering suffixes
	- Many ICs have suffixes indicating tape/reel, temperature range, or lead finish (e.g., `MCP73871T-2ACI/OT`). Match the functional PDO; for PCB footprint purposes the base package is sufficient.
	- If multiple package variants exist, choose the one that matches the footprint you plan to use.

4. Verify mechanical drawing / land pattern
	- Open the distributor's `Mechanical` or `Datasheet` tab and download the mechanical drawing/land pattern.
	- Compare pad dimensions to the KiCad footprint you plan to use; adjust footprint if necessary.

5. For passive or generic items (resistors, capacitors, TVS, USB‑C receptacles)
	- Filter by package (0402/0603/0805/1206), voltage/capacitance or TVS standoff voltage, and series (for polyfuse choose hold current and package).
	- For USB‑C connectors, filter by mounting style (side-entry/bottom-entry), number of pads, and mechanical height.

6. For modules (ESP32 modules)
	- Search by module family (e.g., `ESP32-C6 module`) and vendor; prefer modules with vendor-provided mechanical drawing and PCB landpattern.
	- Confirm antenna type (integrated vs U.FL) and required keepout area.

7. Save the distributor P/N & footprint mapping
	- Record: Manufacturer P/N, Distributor P/N, footprint name, package, and links to the datasheet + mechanical drawing.
	- Create a CSV row for each part so you can import into procurement or KiCad BOM manager.

Example quick-search strings
- `MCP73871 SOT-23-5 Microchip` (charger)
- `MAX6675 SOIC-8 Analog Devices` (thermocouple IC)
- `FS8205A dual MOSFET SOP-8` (protection FET)
- `USB-C receptacle side entry SMD` (connector)

If you want, I can perform the mapping for you (Digikey preferred) and produce the verified CSV; say "Map to DigiKey" and I'll run that next (note: Digikey sometimes presents bot checks which may require manual verification if they trigger during automated fetches).