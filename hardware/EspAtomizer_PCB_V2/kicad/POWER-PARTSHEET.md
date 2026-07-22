# Power Partsheet — ESPAtomizer v2

This partsheet collects suggested manufacturer P/Ns and datasheet links for the v2 power-stage (charger, protector, regulator, connectors).

Key items
- **MAX6675** — thermocouple-to-digital converter. Suggested: `MAX6675CPA+`. Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX6675.pdf
- **MCP73871** — 1S Li‑Ion charger with system power‑path. Suggested: `MCP73871T-2ACI/OT`. Datasheet: https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/DataSheets/MCP73871.pdf
- **Battery protection** — `DW01` + `FS8205A` dual MOSFET (common 1S protection combo). Verify footprints and matching nets.
- **Battery connector** — JST-PH 2-pin (`SM02B-PASK-2.54DSA(21)` or similar) unless you prefer screw/terminal.
- **USB-C connector** — pick side-entry or top-entry to match enclosure. Use CC resistor network per USB Type‑C spec.

Notes
- These are suggested starting parts. Verify mechanical footprints before finalizing PCB layout.
- For battery percentage accurate reporting, consider adding a fuel-gauge IC (e.g., MAX17055) instead of relying purely on ADC estimation.
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

... (trimmed for brevity in repo copy; full notes are in original README/POWER-REVISION.md)
