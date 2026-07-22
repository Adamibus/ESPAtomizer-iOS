# Vendor footprint import (ESPAtomizer)

Place the manufacturer-supplied KiCad board or footprint files here so we can extract the ESP32C6/XIAO footprint into the project library.

Supported inputs
- .kicad_pcb containing the footprint (multi-board is fine)
- .kicad_mod single footprint file
- .pretty directory of footprints

How we’ll use it
1. If you provide a .kicad_pcb:
   - We’ll open it in KiCad PCB Editor, select the ESP32C6/XIAO footprint, and Export Footprint… to our local library at `lib/footprints/XIAO.pretty/`.
   - We’ll save as `XIAO_Socket_2x7_17.78mm_vendor` (or similar).
   - We’ll clone that footprint and add pads 15/16 (BAT+L/BAT-L) for pogo contacts, saving as `XIAO_Socket_2x7_17.78mm_vendor_bat`.
   - We’ll update the schematic symbol J4 to reference the new vendor_bat footprint and ensure pins 15/16 connect to BAT+/BAT-.
2. If you provide a .kicad_mod or .pretty:
   - We’ll copy/import it directly into `lib/footprints/XIAO.pretty/` and perform the same 15/16 pogo-pad augmentation.

What we need from you
- The vendor file(s) placed in this folder.
- If possible, the footprint’s reference or obvious name on the vendor board so we extract the correct one (if multiple similar footprints exist).

After import
- We’ll run Update PCB from Schematic, place the footprint, check courtyard/clearances, and run DRC.
- We’ll keep the vendor-original footprint intact and publish a `*_vendor_bat` variant with integrated pogo pads so you can choose either.
