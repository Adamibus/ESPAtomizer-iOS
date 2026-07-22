# Initial PCB placement plan (ESPAtomizer)

Board outline: 100 mm (X) × 80 mm (Y), origin at lower‑left (0,0) as in Edge.Cuts.
Units below are millimeters.

Goal: Provide starting coordinates/orientations so you can “Update PCB from Schematic” in KiCad and quickly arrange the parts. Fine‑tune as needed.

## References
- U2: XIAO ESP32‑C6 composite module (XIAO:XIAO_ESP32C6_SocketModule)
- J2: OLED socket (STDLIB:PinSocket_1x04_P2.54mm_Vertical)
- J4: MAX6675 socket (STDLIB:PinSocket_1x05_P2.54mm_Vertical)
- J3: JST‑PH battery (STDLIB:JST_PH_B2B‑PH‑K_1x02_P2.00mm_Vertical)
- J5: Heater output terminal (STDLIB:TerminalBlock_Phoenix_MKDS-1,2-2_5.08mm)
- SW2: Battery switch (STDLIB:SW_SPST_Slide)
- SW3: Rotary encoder (ENCODER:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm)
- TP2/TP3: Backside BAT+L / BAT−L (STDLIB:TestPoint_Pad_D2.0mm)
- TP4/TP5: Pogo pads under XIAO (POGO:Pogo_THT_P75_D1.3mm) — BAT (pad 32) / GND (pad 33)
 - TP4/TP5: Pogo pads under XIAO (POGO:Pogo_THT_P75_D1.3mm) — BAT (pad 32) / GND (pad 33)

## Suggested placements

- XIAO composite module (U2)
  - Internal header row spacing (center‑to‑center): 17.78 mm (0.700").
  - Place U2 so the silk “XIAO KEEP‑OUT” box sits centered between header rows; keep tall parts out of this region.
  - Orientation: keep pads stacked along Y (rotation 0° typical for the provided footprint). Ensure pin‑1 indicators align with your silk.

- OLED (J2)
  - Common 0.96" SSD1306 module outline ≈ 27 × 27 mm; allow 28 × 28 mm keep‑out above the socket.
  - Example: place J2 at (80, 16), rotate 180° so the header faces downward into the board.
  - Silkscreen order left→right: 3V3 GND SDA SCL.

- MAX6675 (J4)
  - Typical module outline ≈ 33 × 16 mm (5‑pin). Provide 35 × 18 mm keep‑out above the socket.
  - Example: place J4 at (80, 36), rotate 180° similar to OLED; keep away from MOSFET drain/heater copper.
  - Label pins: 3V3, GND, MAX_SCK, MAX_CS, MAX_SO.

- Rotary encoder (SW3, board‑mounted)
  - Use the vertical EC11/PEC11R footprint; keep a 12 × 12 mm keep‑out cylinder for the shaft/knob and ~7 mm clearance above PCB.
  - Example: near a board edge for access, e.g., center at (80, 62), rotate 0°.
  - If panel‑mounting via cable, you may substitute a 1×5 header (A C B SW GND) with clear silk labels.

- Battery (J3) and switch (SW2)
  - J3 (JST‑PH) at (12, 72), rotate 0° facing board edge
  - SW2 at (30, 72), rotate 0°; align for easy finger access from the same edge
  - Route: J3 BAT+ → SW2 → BAT+ rail → U2 BAT pad (direct trace to JST is recommended for permanent battery attachment; TP4 pogo pads may be left as optional service pads); J3 BAT− → BAT−/GND → U2 GND

- Test pads
    - Keep TP4/TP5 pogo alignment under U2 per CONNECTORS.md; verify stack height and clearance. TP2/TP3 backside permanent lead pads are optional and not required when using the JST pad (J3) for permanent wiring.

## Routing notes

- Heater loop (LiPo+ → heater+ → heater− → MOSFET D → MOSFET S → GND) should be short and wide (≥2–3 mm on 1 oz copper).
- Star the ground near MOSFET S; keep I2C/sensors on the quieter side of the board.
- Keep thermocouple breakout (J4) away from fast edges (MOSFET drain trace) and heater copper pour.
- Place the gate series resistor (R3) and pulldown (R2) right at the MOSFET footprint pads.
- BAT rail behavior: the XIAO BAT pad (pad 32) is bidirectional (charge/discharge). Keep BAT traces short and appropriately wide. If using pogo pins under the XIAO, confirm tip height and force; consider SW2 inline on BAT+ for a hard cutoff. Never tie BAT to 5 V (VBUS).

## After import

1. In KiCad PCB Editor: Tools → Update PCB from Schematic (F8)
2. Place footprints approximately per the coordinates above.
3. Set net classes and clearances if needed; pour GND; maintain keep‑out under the XIAO.
4. Save and iterate. Adjust for your enclosure and connector access.
