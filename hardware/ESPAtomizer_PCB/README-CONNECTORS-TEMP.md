# Connectors placed in KiCad (current quick guide)

This checklist reflects the current schematic annotations. We now place a composite XIAO module symbol/footprint and keep optional discrete sockets as BOM-only (virtual) if needed.

## Present in schematic

1) XIAO ESP32‑C6 composite module
- Designator: U2
- Symbol: XIAO:XIAO_ESP32C6_Module
- Footprint: XIAO:XIAO_ESP32C6_SocketModule (single composite footprint)
- Notes: Provides both header rows and alignment for underside pogo pads to BAT/GND.

2) OLED I2C header (SSD1306 128×64)
- Designator: J2
- Symbol: Connector_Generic:Conn_01x04
- Footprint: STDLIB:PinSocket_1x04_P2.54mm_Vertical
- Silk order (left→right): 3V3, GND, SDA, SCL

3) MAX6675 thermocouple breakout header
- Designator: J4
- Symbol: Connector_Generic:Conn_01x05
- Footprint: STDLIB:PinSocket_1x05_P2.54mm_Vertical
- Nets: 3V3, GND, MAX_SCK, MAX_CS, MAX_SO

4) Battery input (JST‑PH 2‑pin)
- Designator: J3
- Symbol: Connector_Generic:Conn_01x02
- Footprint: STDLIB:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical
- Pins/nets: BAT+, BAT−; BAT+ may route through SW2 before joining BAT rail

5) Battery disconnect switch (SPST)
- Designator: SW2
- Symbol: Switch:SW_SPST
- Footprint: STDLIB:SW_SPST_Slide
- Net: inline on BAT+ between J3 and BAT rail (optional but recommended)

6) Battery test/lead pads and pogo
- TP2 (BAT+L) and TP3 (BAT−L): backside test pads (STDLIB:TestPoint_Pad_D2.0mm)
- TP4 (BAT pad 32 pogo) and TP5 (GND pad 33 pogo): under‑module pogo pins (POGO:Pogo_THT_P75_D1.3mm)

7) Heater output terminal
- Designator: J5
- Symbol: Connector_Generic:Conn_01x02
- Footprint: STDLIB:TerminalBlock_Phoenix_MKDS-1,2-2_5.08mm
- Pins/nets: HEATER+ and HEATER− (MOSFET Drain side)

## Nets to label (for clarity and routing)
- Power rails: 3V3, GND, BAT+, BAT−, BAT_ADC
- I2C: SDA, SCL (to J2 OLED)
- MAX6675: MAX_SCK, MAX_CS, MAX_SO (to J4)
- MOSFET driver: MOSFET_GATE (from U2, GPIO16) with R3 series (100 Ω) and R2 pulldown (100 kΩ)
- Heater terminal: HEATER+, HEATER−
- Pogo: TP4=BAT (pad 32, bidirectional), TP5=GND (pad 33)
- Backside pads: TP2=BAT+L, TP3=BAT−L

## KiCad steps (schematic → PCB)
1) Open ESPAtomizer.kicad_sch and confirm U2, J2, J3, J4, J5, SW2, TP2, TP3, TP4, TP5 are present.
2) Tools → Update PCB from Schematic (F8) to bring the composite module footprint (and remove any legacy discrete row footprints if marked virtual).
3) Place footprints roughly per PLACEMENT.md.
4) Route BAT+, BAT−, and the heater loop; keep the heater loop short and wide.

## Next steps
- Verify pogo heights vs socket stack if using pogo under the module; adjust keep‑outs.
- Place/size copper for MOSFET thermal needs, and add optional flyback diode if switching inductive loads.
- See docs/ENCODER.md for mechanical/placement notes on SW3.
