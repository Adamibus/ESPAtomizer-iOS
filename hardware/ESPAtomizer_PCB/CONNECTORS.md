# Connectors and Sockets (ESPAtomizer PCB)

This document defines the socket-mounted connectors for the XIAO ESP32‑C6, OLED, a K‑type thermocouple breakout (MAX6675), and a rotary encoder (Adafruit PID 377 / EC11‑style) to reduce cost and allow easy replacement. It also defines a baseboard battery connector that ties into the XIAO’s BAT pads.

## Summary

- XIAO ESP32‑C6: Composite module footprint (single footprint `XIAO:XIAO_ESP32C6_SocketModule` placed via symbol U2) providing both header rows plus BAT/GND pogo alignment; legacy separate 1×7 sockets can remain in BOM as virtual parts if desired.
- OLED (SSD1306 I2C): 1 × 1×4 female pin socket (2.54 mm) labeled 3V3/GND/SDA/SCL (designator J2 in schematic).
- MAX6675 breakout: 1 × 1×5 female pin socket (2.54 mm). Pins: 3V3, GND, MAX_SCK, MAX_CS, MAX_SO (MISO). 3.3 V logic (designator J4).
- Rotary encoder (Adafruit 377 / EC11): board‑mounted footprint with integrated switch (designator SW3) or a header pair if panel‑mounted.
- Heater output: 2‑pin 5.08 mm screw terminal (designator J5) carrying HEATER+ / HEATER−.
- Battery input: JST‑PH 2‑pin connector (designator J3) labeled BAT+ / BAT− feeding the BAT rail; pogo and backside pads provide alternate access.

Standard socket height (8.5–9 mm) is suitable for most breakouts. Verify module thickness and any underside components.

## KiCad Footprints (recommended)

- XIAO module: `XIAO:XIAO_ESP32C6_SocketModule` composite footprint (symbol U2) – encapsulates both header rows plus underside pogo alignment; if keeping discrete sockets in BOM, mark them virtual so only the composite footprint is on the PCB.
- OLED socket: STDLIB:PinSocket_1x04_P2.54mm_Vertical
- MAX6675 socket: STDLIB:PinSocket_1x05_P2.54mm_Vertical
- Rotary encoder (board‑mounted option): Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm (or PEC11R equivalent in library)
- Rotary encoder (cabled option): Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical (A, C, B, SW, GND), or split as 1×3 (A,C,B) + 1×2 (SW,GND)
- Heater/LiPo terminals: STDLIB:TerminalBlock_Phoenix_MKDS-1,2-2_5.08mm or equivalent
- MOSFET: STDLIB:TO-220-3_Vertical (if through-hole)
- Battery switch: STDLIB:SW_SPST_Slide (or other low-cost SPST) inline on BAT+
- Battery tie-in: STDLIB:TestPoint_Pad_D1.5mm (TBAT+ / TBAT−) near XIAO sockets for short jumpers to the XIAO BAT pads
  - Optional: pogo pin footprints aligned to the XIAO BAT pads (verify spacing from the official mechanical drawing)
- Battery connector footprint: STDLIB:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical

### Backside battery pads (behind XIAO) — optional

Two small solder/test pads on the PCB back side can be provided for direct battery lead access (labeled BAT+L and BAT‑L). These are intended as optional service points only — for permanent wiring use the JST‑PH connector (`J3`) as the primary battery attachment point.

Notes:
 - Schematic/service references: TP2/TP3 are commonly used for backside permanent pads (optional). TP4/TP5 are the pogo THT footprints under the module and are intended for temporary/service connections.
 - Footprint (if used): `TestPoint:TestPoint_Pad_D2.0mm` (round) for robust soldering/probing.
 - Placement guidance: place pads on B.Cu near the XIAO but keep clearance from the module underside and pogo pads; use short, wide traces (≥0.8–1.0 mm) to BAT rails if you populate these pads.

#### Pogo alignment mapping (under composite module)

Using the vendor SMD footprint `XIAO-ESP32-C6-SMD.kicad_mod` as reference, underside pad centers for battery access:

- BAT (pad 32): (x=7.1086, y=−4.9683)
- GND (pad 33): (x=9.6486, y=−4.9683)

Origin: module top‑left; x→right, y→down (negative y values indicate up from origin inside KiCad file).

Power directionality:
- The XIAO BAT pad (pad 32) is bidirectional (charge/discharge). Treat pogo/test pads tied to BAT as the battery rail. Never tie BAT to 5 V (VBUS).

Resulting mapped pogo pad centers on current PCB (after affine conversion):
- BAT pogo ≈ (26.911, 33.652)
- GND pogo ≈ (29.705, 33.652)

Implemented as THT pogo footprints (`POGO:Pogo_THT_P75_D1.3mm`) with references TP4 (BAT) and TP5 (GND) in schematic (verify final designators—current schematic uses TP4 for BAT pad 32 and TP5 for GND pad 33). Adjust ±0.2 mm if socket stack-up differs.

### Backside lead pads (optional permanent battery leads)

Two backside test/lead pads (TP2/TP3) are optional and may be populated when you want a permanent wire solder point on the back of the PCB. If you plan to permanently attach a LiPo, we recommend using the JST‑PH connector (`J3`) on the board edge instead — it is easier to access and less likely to be damaged by handling.

If you do populate TP2/TP3:
 - Use `STDLIB:TestPoint_Pad_D2.0mm` (round) and keep them offset from TP4/TP5 pogo pads.
 - Make traces wide (≥0.8–1.0 mm) from the pads to the BAT rails.

### Vendor XIAO ESP32‑C6 SMD footprint (direct mount)

- Added a vendor-accurate module footprint for soldering the XIAO directly to the PCB:
  - Path: `hardware/ESPAtomizer_PCB/lib/footprints/XIAO.pretty/XIAO-ESP32-C6-SMD.kicad_mod`
  - Based on Seeed’s published board file; includes accessory pads 24–33 (BAT and GND included).
- Use this only if you intend to mount the module without the two 1×7 sockets.
- Battery access:
  - The vendor SMD footprint exposes a BAT pad (pad 32). You can route this to your existing BAT+L/BAT‑L test pads or a dedicated connector.
  - If we want the integrated “Pad 15/16” battery pogo concept on the SMD variant as well, we’ll create a `XIAO-ESP32-C6-SMD_vendor_bat` variant that adds two dedicated pads in a mechanically safe region around the module outline.

Height notes (stack and compression)
- Typical 2.54 mm female sockets are ~8.5–9.0 mm tall. With a 1.6 mm PCB and 0.5–1.0 mm compression target, pogo tip working height needs to be ~9–10 mm above the base PCB.
- Select pogo series/heads accordingly (e.g., P75 family has multiple head and overall lengths). Verify in-hand parts and adjust footprint keepouts and 3D height if needed.

## Net labels and intended signals

U2 (XIAO composite module):
  - Power: 3V3, GND, VBUS (5V), BAT (pad 32 pogo), GND (pad 33 pogo)
  - I/O used: ENC_A (GPIO0), ENC_B (GPIO1), ENC_SW (GPIO2), BAT_ADC (GPIO21), SDA (GPIO22), SCL (GPIO23), MOSFET_GATE (GPIO16), MAX_SCK (GPIO18), MAX_CS (GPIO20), MAX_SO (GPIO19)
  - Spare: GPIO17 (D7/RX) currently unused

- OLED socket (J3):
  - 3V3, GND, SDA (GPIO22), SCL (GPIO23)
  - Board silk: "3V3 GND SDA SCL" left→right. Verify your OLED header order before plugging.

MAX6675 socket (J4):
  - 3V3, GND, MAX_SCK, MAX_CS, MAX_SO
  - Defaults in firmware (current sketch): SCK=GPIO18, CS=GPIO20, SO=GPIO19 (changeable via defines)

Rotary encoder (SW3 board‑mounted):
  - A → ENC_A (GPIO0)
  - B → ENC_B (GPIO1)
  - C → GND
  - Switch S1/S2 → ENC_SW (GPIO2) and GND
  - No VCC required; internal pull‑ups enabled in firmware.

Battery connector (J3 JST‑PH):
  - BAT+, BAT− (BAT+ may route through SW2 before joining BAT rail if switch populated)
  - TP2 / TP3 backside pads (BAT+L / BAT−L) and TP4 / TP5 pogo pads (BAT / GND) provide alternative access.

## Mechanical notes

XIAO header row spacing: 17.78 mm (0.700") center‑to‑center per Seeed mechanical drawing (captured within composite footprint). Keep‑out under module maintained by silk box.
- Place OLED and MAX6675 sockets near the board edge for easy insertion/removal.

## BOM suggestions (generic)

- Female pin sockets 2.54 mm, 1×7 (qty 2) + 1×5 (qty 1): e.g., Harwin M20‑series, Amphenol FCI 75869‑107LF, or generic.
- Female pin socket 2.54 mm, 1×4 (qty 1): same series as above.
- Rotary encoder w/ push switch (qty 1): EC11/PEC11R/Adafruit PID 377; if panel‑mounting, add matching 1×5 or 1×3+1×2 male headers and cable.
- 2‑pin terminal blocks 5.08 mm (qty 2): Phoenix MKDS 1,2/2‑5,08 or compatible.
- JST‑PH 2‑pin connector (qty 1): J_BAT battery input (B2B‑PH‑K or equivalent)
- Test pads for TBAT+ / TBAT− (qty 2): 1.5–2.0 mm round pads
- Optional pogo pins: per your preferred series; ensure mechanical fit and height
- Hardware battery switch (qty 1): low-cost SPST slide or rocker switch with suitable footprint
- Optional right‑angle variants if you prefer side‑entry modules.

## Notes

- All sockets are oriented vertical. If you need low profile, consider low‑stacking female headers.
- Double‑check OLED pin order (some modules swap VCC/GND). The PCB is labeled; connect accordingly.
