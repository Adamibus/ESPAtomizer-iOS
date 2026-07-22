# Schematic nets reference (MOSFET, Heater, I/O)

Use this as a quick reference to align net labels and designators in the schematic with the current annotations.

## Heater and MOSFET stage

Symbols and designators (as in schematic):
- Q2 (Transistor_FET:Q_NMOS_GDS, TO-220-3 Vertical)
- R3 (100 Ω) gate series resistor
- R2 (100 kΩ) gate pulldown to GND
- J5 (HEATER, 2-pin terminal)

Nets to connect:
- J5 pin 1 = HEATER+ (to battery positive rail)
- J5 pin 2 = HEATER− (to MOSFET Drain)
- Q2 Drain (D) → HEATER− (J5 pin 2)
- Q2 Source (S) → GND (star point with ESP ground)
- Q2 Gate (G) → R3 → MOSFET_GATE (driven from the XIAO module)
- Gate pulldown: Q2 Gate → R2 → GND

Labels to add/verify:
- HEATER+ on J5 pin 1 and battery positive distribution where it feeds the heater
- HEATER− on J5 pin 2 and on Q2 Drain
- MOSFET_GATE on the driver net into R3 (from the XIAO’s GPIO16)
- GND on Q2 Source and R2 bottom node (common ground)
- BAT+ on the battery positive rail (after SW2 if using the disconnect switch)

Optional (for inductive loads):
- If you plan to drive a fan later, add a Schottky diode footprint (SS14/1N5819) across J5 with anode at HEATER− and cathode at HEATER+.

## Sensors, peripherals, and power nets
- J2 OLED (I2C): 3V3, GND, SDA, SCL
- J4 MAX6675 (SPI-like, no MOSI): 3V3, GND, SCK, CS, SO
  - These are labeled MAX_SCK, MAX_CS, MAX_SO in the schematic and are wired to the XIAO’s right-side header pins via the composite module symbol.
- Battery input: J3 BAT+ / BAT− (JST-PH). BAT+ → SW2 → BAT+ rail; BAT− → BAT−/GND
- Backside battery pads (test pads): TP2 = BAT+L, TP3 = BAT−L
- Pogo under-module pads: TP4 = BAT (pad 32, bidirectional), TP5 = GND (pad 33)

## Rotary encoder (Adafruit 377 / EC11 with push switch)

Symbol: Device:Rotary_Encoder_Switch (SW3 in the schematic)

Pins/nets:
- A → ENC_A
- B → ENC_B
- C (common) → GND
- S1/S2 (switch) → ENC_SW and GND (wire one side of the push switch to ENC_SW, the other to GND)

Notes:
- Use the ESP32’s internal pull‑ups on A/B/SW (firmware sets INPUT_PULLUP).
- No external resistors required for the encoder; optional 100 nF from C→A and C→B for extra hardware debounce.

## After wiring in schematic
1. Annotate and save.
2. Tools → Update PCB from Schematic (F8).
3. Place Q2 with copper area for heat; route the HEATER loop wide (≥2–3 mm on 1 oz).
