# ESPAtomizer PCB (KiCad skeleton)

This is a minimal KiCad 6/7 project skeleton. Open `ESPAtomizer.kicad_pro` in KiCad.

> Note: For local firmware build/upload verification and examples, see [tools/README.md](../../tools/README.md). It documents `tools/verify_xiao_variants.ps1`, a PowerShell helper that builds (and optionally uploads) the firmware for both XIAO ESP32-C3 and XIAO ESP32-C6 environments.

## Next steps

1. Schematic
   - Open `ESPAtomizer.kicad_sch`
   - Core symbols now used:
       - U2: `XIAO:XIAO_ESP32C6_Module` (composite symbol) with footprint `XIAO:XIAO_ESP32C6_SocketModule` (single footprint including both header rows)
       - J4: MAX6675 1×5 socket (3V3, GND, MAX_SCK, MAX_CS, MAX_SO)
       - J2: OLED 1×4 I2C socket (3V3, GND, SDA, SCL)
       - J3: JST‑PH battery input (BAT+, BAT−)
       - SW2: Battery SPST switch inline on BAT+
   - TP4 / TP5: Pogo pads to XIAO BAT (pad 32) and GND (pad 33), bidirectional BAT rail
       - Q2: MOSFET (`Transistor_FET:Q_NMOS_GDS`) + R3 (100 Ω gate series) + R2 (100 kΩ gate pulldown) + J5 heater terminal (HEATER+ / HEATER−)
       - SW3: Rotary encoder (`Rotary_Encoder_Switch`) A/B/ENC_SW nets to GPIO0/1/2
   - Net labels: MOSFET_GATE (GPIO16), MAX_SCK (GPIO18), MAX_CS (GPIO20), MAX_SO (GPIO19), ENC_A (GPIO0), ENC_B (GPIO1), ENC_SW (GPIO2), BAT_ADC (GPIO21), SDA/SCL (GPIO22/23), BAT+/BAT−, HEATER+/HEATER−.
   - Note: flyback diode not required for the current resistive heater; omit unless you later add inductive loads.

2. Board
   - Open `ESPAtomizer.kicad_pcb`
   - Import netlist (or Update PCB from Schematic)
    - Set board outline to your enclosure; current placeholder is 100×80 mm
   - Route high‑current heater loop with wide copper (≥2–3 mm at 1 oz for ~4 A)
   - Pour GND plane; keep thermocouple inputs away from MOSFET drain trace
    - Place two XIAO socket rows with the correct center‑to‑center spacing (use ~15.5 mm C‑C per your updated measurement; verify on your board). Leave keepout under module.
   - Place J_BAT near board edge; route BAT+ from J_BAT through SW1 before joining TBAT+ to the XIAO BAT+

3. Powering from 1S LiPo
   - Bring BAT+ / BAT− from J3 into the board, then to the XIAO BAT pad via a direct trace and common GND. Use the JST pad (J3) for permanent battery attachment; TP4/TP5 pogo pads remain optional service points.
   - Add the hardware battery switch SW2 inline on BAT+ (full disconnect when OFF)
   - Optional reverse‑polarity protection (Schottky or ideal‑diode controller)
   - Keep ESP GND and heater return tied at a solid star point

4. Test points & silkscreen
   - TP: GATE, DRAIN, SOURCE, 3V3, VBAT, SDA, SCL, SCK, CS, SO
   - Label connectors: HEATER +/−, LiPo +/−, OLED (3V3/GND/SDA/SCL), MAX6675 (3V3/GND/SCK/CS/SO), J_BAT (BAT+/BAT−)
   - Add “Socketed” on silk near the XIAO, OLED, and MAX6675 footprints for assembly clarity

5. Fabrication
   - 2‑layer, 1 oz copper is fine; choose 2 oz if you want extra margin for heater currents
   - Keep creepage around battery connector; avoid thin necks on power pours

Refer to `docs/PCB-PROTOTYPING.md` for detailed layout and bring‑up notes.

## Socket-mount connector map (draft)

- U2: XIAO ESP32‑C6 composite module. Nets include: 3V3, GND, VBUS (5V), BAT (pad 32), BAT_ADC (GPIO21), MOSFET_GATE (GPIO16), ENC_A (GPIO0), ENC_B (GPIO1), ENC_SW (GPIO2), SDA (GPIO22), SCL (GPIO23), MAX_SCK (GPIO18), MAX_CS (GPIO20), MAX_SO (GPIO19), plus spare GPIO17.
- J3: OLED socket (1×4): 3V3, GND, SDA, SCL.
- J4: MAX6675 socket (1×5): 3V3, GND, SCK, CS, SO.
- J3: J_BAT battery input (2×1): BAT+ / BAT− (BAT+ passes through SW2). Use the JST pad (J3) as the permanent battery attachment point; TP backside pads are optional.
 
## Firmware pin mapping and overrides

- **Authoritative source:** `ESPAtomizer/config.h` is the single source of truth for board pin assignments used by the firmware. Keep pin changes centralized there to avoid divergence between schematic/PCB and code.
- **Build-time overrides:** You can override pins at compile time instead of editing `config.h` directly. Examples:
   - PlatformIO (add to `platformio.ini` under your environment):
      - `build_flags = -DOUTPUT_PIN=5 -DD10_PIN=19`
   - Arduino CLI:
      - `arduino-cli compile --fqbn <your_fqbn> -DOUTPUT_PIN=5 -DD10_PIN=19 path/to/ESPAtomizer`
- **Why this matters:** The schematic and footprint may use different conventions for labeling header pins (D‑numbers vs physical pads). `config.h` maps the correct physical GPIOs from the PCB; prefer changing `config.h` or using build flags rather than editing the main sketch.

## Review findings (2025-11-26)

- **Critical pin-mapping mismatch:** The PCB currently routes the MOSFET gate net (`MOSFET_GATE`) to **Pad 1 / GPIO0** on the XIAO footprint, while the schematic/docs state `MOSFET_GATE` should be driven from **GPIO16**. GPIO0 is a boot/strapping-related pin on Espressif modules and driving or pulling it during reset can prevent normal boot (puts the chip in download/boot mode). Do not fabricate without resolving this.

- **Immediate recommendation:** Reassign `MOSFET_GATE` to the non-strapping pin (GPIO16 / Pad 7) to match documentation and avoid boot issues. If you cannot move the net, re-evaluate the pull resistor strategy on GPIO0 (very small chance acceptable only after consulting Espressif strapping rules) — but changing to GPIO16 is the safer, preferred fix.

- **MOSFET gate & heater current:** The BOM warns to verify the MOSFET's RDS(on) at VGS = 3.3 V. Confirm the chosen MOSFET is guaranteed to fully switch at 3.3 V (logic-level device). If not, replace with a MOSFET specified for low RDS(on) at 2.5–3.3 V or add a gate driver.

- **BAT_ADC (GPIO21) and protection:** `BAT_ADC` is connected to `GPIO21` per schematic. Verify `GPIO21` supports ADC on the ESP32‑C6 variant used in the XIAO module and ensure a proper resistor divider, input clamp/TVS, and C (10 nF recommended) are present to protect the ADC input from battery voltage.

- **Power & decoupling:** Confirm presence of an LDO/regulator if you expect the module to be powered from BAT directly to 3.3 V. Ensure adequate local decoupling caps placed close to the module's `3V3` pad (C0G/NP0 and bulk electrolytic as needed). I did not find a dedicated LDO footprint in the quick pass; verify whether the XIAO module expects an external 3.3 V supply or provides internal regulation.

- **RF / antenna keepout:** The XIAO module likely contains the RF antenna — keep the placement notes and antenna keepout from `PLACEMENT.md` enforced. Verify there is no copper or ground pours in the antenna keepout area on the PCB revision.

- **Assembly / placement notes:** Place gate series resistor (`R3`) and gate pulldown (`R2`) close to the MOSFET pads as documented. If you move the MOSFET gate net to GPIO16, also update `config.h` and any firmware references.

- **Do not release to fabrication yet:** Fix the `MOSFET_GATE` mapping (and optionally verify gate/pull resistor values, MOSFET RDS(on) at 3.3 V, and BAT_ADC clamps) before ordering boards.

### Next actions

- Update the KiCad schematic and PCB to move `MOSFET_GATE` from Pad 1 (GPIO0) to Pad 7 (GPIO16), then run ERC and update PCB from schematic.
- Fetch and cross-check Espressif ESP32‑C6 hardware design guide for boot strapping/pin requirements and confirm any required pull resistor values for strapping pins.
- Verify MOSFET selection in `bom-mosfet-alternates.csv` or replace with a guaranteed logic-level MOSFET if needed.
- Confirm power regulator topology and add/verify decoupling on the `3V3` rail.

If you want, I can prepare the KiCad schematic patch to reassign the MOSFET gate net to Pad 7 (GPIO16) and update `config.h` accordingly; tell me to proceed and I will make the edits and run another pass of checks.

