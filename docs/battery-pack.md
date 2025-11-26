# Battery pack procurement (Option 1)

This document lists vetted candidate parts for a safe single-cell 18650 option (Option 1) for bench testing the ESPAtomizer heater. It contains links found and a short vendor/safety checklist.

## Summary recommendation
- Cell options (choose 1): Samsung INR18650-25R (20A, 2500mAh) or Molicel P26A (35A, 2600mAh). Prefer Molicel P26A if you expect heavy pulses and sustained high currents. Buy from a reputable vendor (Illumn, IMR Batteries, BatteryJunction, or authorized distributers).
- Connectors / pigtails: use XT30 (16–18 AWG) for compact builds, XT60 for higher margin. Prefer 16 AWG silicone wire attached to XT30/XT60 pigtails.
- Protection: add a 1S PCM rated >= continuous current and able to handle pulses (20–30A rating), and an inline blade fuse (ATC/ATO) sized for continuous current with pulse headroom (recommend 15 A baseline; use 20 A if you expect larger pulses).

## Vetted candidate product links

### Cells
- Samsung INR18650-25R (example vendor page):
  - Illumn: https://www.illumn.com/products/samsung-18650-inr18650-25r-2500mah-20a?utm_source=google
  - Notes: 2500 mAh, 20 A continuous (datasheet required). Prefer Illumn or similar reputable seller to reduce counterfeit risk.
- Molicel P26A (example vendor page):
  - IMR Batteries: https://imrbatteries.com/molicel-p26a-2600mah-35a-18650-battery
  - Notes: 2600 mAh, 35 A continuous rating (good headroom for pulses).

### Procurement table (cells only — connectors/fuses skipped)
| Cell model | Vendor / link | Approx price (Nov 2025) | Notes |
|---|---|---:|---|
| Samsung INR18650-25R | Illumn — https://www.illumn.com/products/samsung-18650-inr18650-25r-2500mah-20a | $6–$9 ea (estimate) | 2500 mAh, spec'd 20 A continuous — verify datasheet on vendor page. |
| Molicel P26A | IMR Batteries — https://imrbatteries.com/molicel-p26a-2600mah-35a-18650-battery | $8–$12 ea (estimate) | 2600 mAh, higher continuous rating (35 A) — better headroom for preheat pulses. |

_Notes:_
- Prices are approximate ranges and can vary by seller, qty, and promotions — verify on vendor pages before ordering.
- Per your instruction, connector and fuse requirements are omitted from this table. If you want pigtails or protection added later, I can append them.

## Assembly & bench-test checklist (next step)

Purpose: provide a safe, repeatable checklist to assemble a single-cell test pack and bench-test the heater on the ESPAtomizer. This assumes you already purchased cells (above). Connector/fuse procurement is intentionally omitted per project decision, but safety notes are included — do not skip protective practices if you change your mind.

Prerequisites (minimum items you'll need):
- 1× genuine 18650 cell (Samsung 25R or Molicel P26A)
- Insulated cell holder or safe method to make temporary battery connections (spring clip holder, battery bench adapter)
- Multimeter (DC volts, continuity)
- Current measurement: either a DC current clamp capable of the expected current, or a low-value shunt resistor + accurate voltmeter
- Test load capable of reproducing heater current (e.g., adjustable DC electronic load or equivalent resistor bank); for initial checks a bench power supply with current limit and heater connected to MOSFET on board
- Temperature monitoring: IR thermometer or thermocouple on the heater/board; thermal camera optional
- Small insulation materials and heatshrink, high-quality leads for test
- Safety equipment: fire-safe surface, smoke extinguisher nearby, eye protection

Step-by-step bench-test
1. Visual inspection
  - Confirm cell has no dents, swelling, or visible damage. If it does, do not use it.
  - Confirm PCB solder joints at Q2, heavy current traces, and battery connector area appear intact.

2. Make a safe temporary connection
  - Place the cell in an insulated holder or use a dedicated bench adapter. Ensure good polarity identification.
  - Keep all test leads short and use adequately rated leads.

3. No-load checks
  - With cell connected and the device off, measure open-circuit battery voltage with a multimeter to confirm expected voltage (~3.6–4.2 V depending on charge).
  - Measure continuity/connection resistance between battery + and the MOSFET drain input to confirm there are no unintended open circuits.

4. Low-power smoke test
  - Power the ESP32/firmware from the cell (if designed to run from the cell) or apply regulated supply simulating battery voltage. Confirm MCU boots and no abnormal heating.

5. Heater switching functional test (no-thermal load first)
  - With firmware set to a low duty PWM/low preheat value, enable heater output and measure MOSFET gate voltage and drain voltage waveform with an oscilloscope or multimeter (DC). Expect gate to swing near MCU drive (approx 3.3V) when PWM is active; drain should switch between near-battery and near-ground depending on on/off state.
  - If gate never rises, stop and debug MCU pin, gate resistor, or MOSFET orientation.

6. Controlled current test with heater attached
  - Attach the actual heater (or equivalent resistive load matching heater R). Start with short preheat pulses and measure current. Expect steady-state currents in the ~6–8 A range for your heater; short preheat pulses may be higher.
  - Monitor cell voltage sag, connector/lead temperature, and MOSFET temperature. If any component overheats quickly, stop immediately.

7. Thermal monitoring and logging
  - Place an IR thermometer on the heater and board; if using firmware telemetry (BLE/serial), log temperature and battery voltage over a few test cycles.
  - Verify that preheat-to-PID handoff is working (if firmware implements preheat) and heater control stabilizes.

8. Fault tests
  - Test disabling heater on low battery: Simulate lower cell voltage (or discharge to a safe level) and confirm firmware inhibits heating as expected.
  - Check MOSFET thermal run-away by applying repeated preheat pulses; ensure MOSFET and traces maintain safe temps.

9. Post-test inspection
  - Inspect cell and wiring for any signs of heating, insulation damage, or solder melting.
  - If everything passes, document the measured steady-state current, peak pulse current, and observed temperature rise in `docs/battery-pack.md` or a test log file.

Safety notes
- Even for bench tests, avoid leaving an unattended battery under heavy pulses. Use small-duration pulses with cooling periods.
- If you don't plan to add a PCM/fuse before testing, be conservative: smaller pulses, monitor temps closely, and stop on any abnormal sign.
- Prefer reputable cells — counterfeit or low-quality cells can fail catastrophically under high pulse stress.

Deliverables for this step
- A short test log with: cell model, initial voltage, measured steady current, peak pulse current, MOSFET temp at end of test, and any abnormal observations. Add the log to `docs/` as `docs/battery-test-log.md` (I can create a template if you want).

If you want, I will create the `docs/battery-test-log.md` template now and set the TODO `Assemble & bench-test battery pack` to completed once you confirm tests are done and provide the log. Otherwise I will wait for you to run the tests and report results.

### Pigtails / pre-wired leads
- Amass XT30U-M 18AWG 10cm LiPo Pigtail (3-pack) — Amazon ASIN `B09BDHT7LZ`
  - Link: https://www.amazon.com/Amass-XT30U-M-18AWG-10cm-Pigtail/dp/B09BDHT7LZ
  - Notes: 18 AWG; for higher margin use 16 AWG.
- Lumenier XT30 Lipo Pigtail w/ 470µF cap, 16 AWG (3pcs) — Amazon ASIN `B08RS2H111`
  - Link: https://www.amazon.com/Lumenier-Pigtail-470uF-Capacitor-16AWG/dp/B08RS2H111
  - Notes: 16 AWG silicone wire, includes input capacitor (helpful for transient suppression with long leads).

### Inline fuse / protection examples
- 16 AWG Inline ATC/ATO blade fuse holder (10-pack) — Amazon ASIN `B08K3NFZCP`
  - Link: https://www.amazon.com/16-AWG-Inline-Fuse-Holder/dp/B08K3NFZCP
  - Notes: check included fuse amperage. Use 15 A or 20 A blade fuses depending on required headroom.
- Example 1S PCM protection module (generic listing) — Amazon example:
  - Link: https://www.amazon.com/Li-ion-Lithium-Battery-Protection-Circuit/dp/B07W6L7F3X
  - Notes: many generic 1S PCMs exist; verify continuous/pulse ratings and seller reputation.

## Quick vendor/datasheet checks (before purchase)
- Verify manufacturer part numbers and compare datasheet specifications (continuous discharge A, peak/pulse rating, recommended charging/discharging conditions).
- Prefer vendors with clear authenticity guarantees (Illumn, IMR Batteries, BatteryJunction). Avoid unknown marketplace sellers for cells.
- For pigtails, confirm wire AWG (16 AWG recommended), silicone insulation, and good solder/crimp quality.
- For protection, prefer a PCM/BMS with a continuous rating >= expected continuous current and a specified pulse rating that covers preheat pulses.

## Suggested baseline configuration (for your heater ~6–8 A continuous, higher preheat pulses)
- Cell: Molicel P26A (preferred) or Samsung 25R (acceptable).
- Connector/pigtail: XT30 with 16 AWG silicone pigtail (Lumenier 16 AWG pigtail recommended).
- PCM: 1S PCM rated >= 20 A (verify pulse rating). If using a PCM with limited pulse headroom, add an inline fuse sized to protect against sustained overloads.
- Inline fuse: 15 A slow/standard blade fuse (or 20 A if you accept less protective margin for high pulses). Use fuse + PCM together for safety and convenience.

## Next steps / acceptance criteria
1. Confirm whether you prefer `XT30` or `XT60` as final connector choice. I recommend `XT30` + 16 AWG for a compact and reliable solution for ~8 A continuous.
2. If you want, I will convert these candidates into a small procurement table with vendor SKUs, prices, and buy-links, and update `docs/to do` with order/check steps.

---
_Created by automated procurement assist on behalf of ESPAtomizer project. Verify seller availability & datasheets before ordering._
