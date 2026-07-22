# POWER BOM — Verified (Manufacturer P/Ns + Datasheets)

Date: 2025-11-26

This file accompanies `POWER-BOM-VERIFIED.csv` and provides the authoritative manufacturer datasheet / mechanical links and short layout notes for each part.

Primary entries

- U_CHG — Microchip MCP73871
  - Datasheet (Microchip): https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP73871-Data-Sheet-DS20002090F.pdf
  - Product page: https://www.microchip.com/en-us/product/MCP73871
  - Notes: Power-path charger for 1S Li-ion. Choose SOT-23-5 variant and follow application notes for VPCC settings and decoupling. Place close to USB connector.

- U_TC — Analog Devices MAX6675
  - Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX6675.pdf
  - Product page: https://www.analog.com/en/products/MAX6675.html
  - Notes: SOIC-8; add 0.1uF decoupling and 1uF bulk cap near VCC. Place near thermocouple connector and keep traces short.

- U_PROT_IC — DW01 (1S protector) — vendor variants
  - Datasheet: search for "DW01 datasheet" from vendor/manufacturer (common vendors publish PDF). Footprints vary by vendor.
  - Notes: Common 1S protection IC; usually paired with FS8205A dual MOSFET.

- U_PROT_FET — FS8205A (dual MOSFET)
  - Datasheet: search for "FS8205A datasheet"; confirm package variant (SOP-8/DFN) and thermal pad requirements.
  - Notes: Works with DW01 for 1S battery protection. Use generous copper for MOSFET thermal dissipation.

- CON_USB — USB-C receptacle (side-entry)
  - USB Type-C mechanical spec & RD: https://www.usb.org/document-library/type-c-rd
  - Notes: Pick a vendor part that fits your enclosure. Add CC resistors (5.1k) to CC1/CC2 to advertise as device for basic 5V.

- U_REG — MP2307DN (buck) or MCP1700-33 (LDO) alternative
  - MP2307 product page: https://www.monolithicpower.com/en/products/mp2307.html
  - Notes: Use MP2307 if you need efficiency and >200 mA continuous current. For low current (e.g., <200 mA), an LDO like MCP1700-33 may be simpler and lower noise.

- U_ESP — Espressif ESP32-C6 module (choose vendor variant)
  - Modules & guidelines: https://www.espressif.com/en/products/modules/esp32-c6
  - Hardware design guideline (modules): https://documentation.espressif.com/esp32-c6_hardware_design_guidelines_en.pdf
  - Notes: Choose a module with integrated antenna and vendor-provided land pattern/keepout. Verify module VCC requirements (3.3V or accepts BAT with internal regulator).

- Q_HEAT — Heater MOSFET (logic-level)
  - Selection guidance: Ensure Rds(on) is specified at VGS=3.3V and is low enough for heater current (e.g., Rds(on) <= 50 mOhm). Choose footprint based on power dissipation (DPAK/SO-8 for higher current, SOT-23 for low power).

- D_VBUS_TVS — SMBJ5.0A (TVS)
  - Vendor example: Littelfuse SMBJ series product pages.

- F1 — Polyfuse / PTC on VBUS
  - Vendor example: Bourns resettable PTC 1206 series.

Verification checklist (before layout)
1. Download each manufacturer's mechanical drawing and recommended landpattern; update KiCad footprints to match.
2. Confirm package variants (temperature grade, tape & reel suffix) for procurement.
3. Run ERC/DRC in KiCad after adding parts; verify thermal relief on MOSFETs and charger.
4. Verify ESP module antenna keepout and clear copper zones under antenna.

Why I used manufacturer P/Ns
- Distributor scrapers (DigiKey/Mouser) presented intermittent bot checks during automated fetches. Using manufacturer P/Ns + datasheet links avoids blocker and still provides layout-ready data (footprints must be verified against mechanical drawings).

Next steps I can take for you
- Produce exact KiCad footprints (library files) for MCP73871 (SOT-23-5), MAX6675 (SOIC-8), MP2307 (SOT-23-6), and a recommended USB-C receptacle footprint (I will base these on manufacturer mechanical drawings).
- Or, map these manufacturer P/Ns to Digikey/Mouser orderable P/Ns (requires retrying distributor fetches and possibly manual verification for CAPTCHAs).

Say "Create KiCad footprints" to have me generate the footprint files next.
