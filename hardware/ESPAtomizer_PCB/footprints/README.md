This folder contains a small set of generic KiCad footprints used for the ESPAtomizer power-stage and sensors. Add `hardware/ESPAtomizer_PCB/footprints/ESPAtomizer.pretty/` as a KiCad footprint library (Preferences → Manage Footprint Libraries) or copy the `.kicad_mod` files into your global `.pretty` library.

Files added:
- `MCP73871_SOT23-5.kicad_mod` — generic SOT-23-5 footprint for MCP73871 charger.
- `MAX6675_SOIC-8.kicad_mod` — generic SOIC-8 footprint for MAX6675 thermocouple IC.
- `MP2307_SOT23-6.kicad_mod` — generic SOT-23-6 footprint for MP2307 buck regulator.
- `USB_C_Receptacle_SideEntry.kicad_mod` — simplified USB-C side-entry receptacle (generic pad map).

Notes and next steps:
- These footprints are intentionally minimal/generic. Verify dimensions against the selected manufacturer's mechanical datasheet before production.
- Update pad names/nets in your schematic symbols to match these footprint pad numbers if you adopt them (or edit the footprint to match existing symbols).
- If you want, I can refine these footprints to match specific manufacturer mechanical drawings (I recommend doing this for the USB-C connector and MCP73871).
