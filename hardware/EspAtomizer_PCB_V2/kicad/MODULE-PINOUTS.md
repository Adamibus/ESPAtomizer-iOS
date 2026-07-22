# ESPAtomizer v2 — Module & Component Pinouts (reference)

This document lists the modules and key components used for the ESPAtomizer v2 power + sensor board, the functional pins/nets you should expose on the PCB, and short wiring/layout notes to help you pick footprints and begin board layout.

Important: this is a wiring/pinout reference for board layout only. I did not change any `.kicad_sch` or `.kicad_pcb` files. Where exact mechanical pad geometry matters (module and connectors) you must verify pad dimensions against the manufacturer's mechanical drawing before fabrication.

---

**1) ESP32-S3-MINI-1 (module)**
- Purpose: main microcontroller (Wi‑Fi + BLE). Compact, castellated module.
- Nets to expose on board (functional pins):
  - `3V3` (module VCC) — stable 3.3V rail
  - `GND` — ground
  - `EN` / `RESET` — module enable/reset pin (pull-up ~100k recommended, add reset pushbutton)
  - `IO0` (boot) — expose if you want manual/auto-flash capability
  - `TX0`, `RX0` — UART for programming/debug (route to header or USB‑UART chip)
  - SPI host signals (for MAX6675): `SCK`, `MISO` (DOUT/SO), `CS` — pick non‑strap pins for these
  - ADC pin — for battery voltage measurement (choose ADC‑capable pin; paint net `BAT_ADC`)
  - I2C SDA/SCL — optional for other sensors
  - Additional GPIOs for status LEDs, MOSFET gate control, CHG_STATUS inputs
- Notes:
  - Do not connect high‑current or always‑low signals to boot/strap pins — these can change boot behaviour.
  - Place decoupling caps (100 nF + 10 µF) close to module VCC pins.
  - Respect antenna keepout; confirm the module mechanical/datasheet antenna area.

**2) MAX6675 (SOIC‑8)**
- Purpose: K‑type thermocouple to digital converter (SPI slave)
- Pins to route:
  - `VCC` → `3V3` (module VCC)
  - `GND`
  - `SCK` → connect to SPI SCK on the MCU
  - `CS`  → dedicated chip-select GPIO from MCU
  - `SO` (also labeled `DO`) → MISO line to MCU
- Notes:
  - SPI mode: MAX6675 uses CPOL=0, CPHA=0 (standard SCK timing) — ensure MCU SPI settings match.
  - Add 0.1 µF + 10 µF decoupling at VCC.
  - Keep CS and SCK traces short between MCU and MAX6675; keep SO return close.

**3) MCP73871 (SOT‑23‑5 charger with power‑path)**
- Purpose: 1S Li‑Ion charger with system power‑path (allows running while charging)
- Functional pins/nets to wire on board:
  - `VBUS` / `VIN` — USB VBUS (through fuse + TVS)
  - `BAT` — connection to battery positive (goes to battery protector side)
  - `SYS` — protected system rail (to feed system/regulator/3.3V LDO if used)
  - `PROG` — program resistor to set charge current (Rprog)
  - `STAT` — charge status output (optional to MCU)
  - `GND`
- Notes:
  - Add input fuse and VBUS TVS to protect USB VBUS.
  - Follow MCP73871 datasheet for decoupling and placement (caps close to chip).
  - BAT must connect to the battery protector (DW01/FS8205A) as appropriate.

**4) Battery protection: DW01 + FS8205A (1S protector combo)**
- Purpose: cell overcharge/overdischarge and short protection
- Functional nets to wire:
  - `BAT+` — battery positive (JST connector side)
  - `BAT-` — battery negative (connect to GND)
  - `PROTECT_OUT` / `SYS` — protected output that feeds the rest of the board
  - `GND`
- Notes:
  - Place protector as series device between the battery and the system: BAT -> protector -> SYS
  - Keep traces between protector and battery connector short and heavy (wide) for current handling.

**5) USB‑C receptacle**
- Purpose: USB power input (and USB‑UART if routing data lines)
- Pins/nets to wire:
  - `VBUS` — goes to input fuse -> TVS -> MCP73871 VIN
  - `GND`
  - `CC1/CC2` — pull‑resistors (select Rd for device mode). For a device, use Rd (e.g., 5.1kΩ) so upstream supplies VBUS.
  - `D+`/`D-` — optionally route to USB-UART/USB data if you include USB host/device data (not required for charging)
- Notes:
  - Use correct CC resistor values per USB‑C spec (and footprint matching chosen connector).
  - Add VBUS fuse and TVS.

**6) MP2307 (SOT‑23‑6) or MCP1700 LDO (regulator)**
- Purpose: generate 3.3V for the module and sensors (if you do not rely on on‑module regulator)
- Nets to wire:
  - `VIN` ← `SYS` or `VBUS` (per design choice)
  - `VOUT` → `3V3`
  - `GND`
  - `EN` / `SHDN` — optional enable pin to MCU
- Notes:
  - Add recommended input and output decoupling per regulator datasheet.
  - If power draw is low, a simple LDO (MCP1700) simplifies layout; for higher loads use buck converter (MP2307) and follow thermal/layout guidelines.

**7) Heater MOSFET (logic‑level MOSFET)**
- Purpose: switch the heater load under MCU control
- Nets:
  - `D` → heater negative or heater return (depending on low‑side/high‑side topology)
  - `S` → ground or battery side depending on topology
  - `G` → MCU GPIO (use a non‑strap pin); add gate series resistor (~100Ω) and gate pulldown (~100k) close to MOSFET
- Notes:
  - Use a logic‑level MOSFET with low Rds(on); place MOSFET near heater connector and provide wide copper for current.
  - Place gate resistor and pulldown close to the MOSFET pad.

**8) Battery connector (JST‑PH 2‑pin)**
- Purpose: connect removable Li‑ion pack
- Nets:
  - `BAT+` → protector -> MCP73871 BAT
  - `BAT-` → GND
- Notes:
  - Place connector near PCB edge for ease of access; route battery high‑current traces short.

---

Quick mapping checklist for layout start:
- Create nets: `3V3`, `GND`, `VBUS`, `BAT+`, `BAT-`, `SYS`, `BAT_ADC`, `SCK`, `MISO`, `CS_MAX6675`, `EN`, `TX0`, `RX0`, `CHG_STAT`.
- Reserve MCU pins: pick ADC-capable pin for `BAT_ADC` and non‑strap pins for `CS_MAX6675` and MOSFET gate.
- Add decoupling: 100 nF + 10 µF at MCU VCC; 0.1 µF + 10 µF at MAX6675; caps at regulator IN/OUT.
- Follow connector datasheets: ensure USB‑C and module keepout/anchor details are respected.

If you want, I can now:
- (A) Generate a compact `PIN-MAP.csv` with suggested MCU pin assignments (candidate GPIOs) for SPI, UART, ADC and MOSFET control (note: I will pick non‑strap pins conservatively and label suggestions). This is a mapping suggestion only and must be validated against your firmware and module pinout.
- (B) Convert this document into a PCB checklist `LAYOUT-NOTES.md` that focuses on component placement, thermal and routing priorities.

Tell me whether you want option A or B next (or both). Also, if you can upload the official `ESP32-S3-MINI-1` mechanical/datasheet I will convert the placeholder footprint into an exact footprint ready for layout.