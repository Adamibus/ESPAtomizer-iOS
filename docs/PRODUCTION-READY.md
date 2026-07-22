# Production-ready checklist for ESPAtomizer PCB

This document lists the recommended steps, checks, and outputs to convert the current KiCad project into production-ready files and a reliable prototype. Follow the items in order; many items are quick checks and should be repeated after edits.

## Quick preamble
 Status: schematic and PCB edits that previously added R4/R5 and the temporary divider BOM have been reverted from the repository (per your request).

 Planned change (outline only): add a battery divider consisting of two 100k resistors so firmware ADC reads the battery.
 - `R4` (100k) between `BAT+` and `BAT_ADC`
 - `R5` (100k) between `BAT_ADC` and `GND`

 This document now serves as the authoritative, textual plan for the intended schematic/PCB changes so you can re-apply them manually in KiCad when convenient.


## Acceptance criteria (what "production-ready" means here)
- Schematic: ERC passes (no unexpected unconnected pins). All net names correct (BAT+, BAT_ADC, GND, MOSFET_GATE, HEATER+, HEATER-).
- PCB: DRC passes with no rule violations; net classes configured for current-carrying nets; footprints match BOM; mechanical constraints (keepouts/mount holes) respected.
- BOM: Complete with manufacturer part numbers (MPN), distributor links, and intended footprints.
- Outputs: Gerber files + drill file + pick-and-place CSV + BOM CSV + assembly drawing + fabrication notes.

## Recommended board-level default specs (industry standard defaults you can change)
- Board thickness: 1.6 mm (or 1.0 mm for thin designs) — confirm with your assembler.
- Copper weight: 1 oz (35 µm) standard. Use 2 oz for high-current heater traces if needed.
- Surface finish: ENIG preferred for small-pitch and assembly reliability; HASL lead-free ok for prototypes.
- Soldermask color: your choice; green default. Silkscreen: white or black depending on mask.
- Minimum trace width/clearance: 6 mil / 6 mil for general signals; for heater traces choose width appropriate to current (see net class below).
- Min drill size: 0.3 mm for vias; through-holes per component (axial resistors will need larger holes sized to vendor leads).

## Step-by-step checklist (detailed)

1) Schematic wiring & ERC
- Wire `R4` (BAT+ → BAT_ADC) and `R5` (BAT_ADC → GND).
- Re-annotate symbols if needed (Tools → Annotate Schematic). Keep reference designators consistent (R4/R5 used already).
- Run ERC (Electrical Rules Check). Resolve any warnings/errors. Common issues: passive pins not connected, ambiguous net ties.

2) Footprint verification
- Verify every symbol has the correct `Footprint` property and exact library:footprint string. Avoid local/unversioned footprints if possible.
- For through-hole axial parts, confirm the footprint is appropriate (hole diameter, pad spacing). The project uses `STDLIB:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal` — confirm lead spacing and hole size match the part you'll source.
- For XIAO module and sockets, ensure the chosen footprint is the one you intend to solder or socket.

3) Update PCB from schematic (netlist)
- In KiCad: Tools → Update PCB from Schematic. Carefully review the list of newly added items before confirming.
- Place the new through-hole resistors near `U2` pad 4; axial parts are large — place them where they will not obstruct connectors, the heater, or mounting holes. Consider placing them on the bottom side if mechanical constraints allow.

4) Net classes and high-current traces
- Set net class for heater traces (e.g., `HEATER`): set minimum width according to expected current and copper weight. Example: 2 A on 1 oz copper → ~1.5 mm width; verify with an online trace width calculator (or manufacturer guidance).
- Set via sizes and annular ring minimums for those nets.

## MOSFET (Q2) — RLB8721PBF (TO-220)

Part: RLB8721PBF, TO-220 package (documented here and included in BOM). Important notes:

- Gate drive and Rds(on): the board currently drives the MOSFET gate from the MCU (3.3 V). Verify the RLB8721PBF datasheet for Rds(on) at logic-level VGS (3.3 V). If the part's Rds(on) is specified only at higher VGS (e.g., 10 V) and is not sufficiently low at 3.3 V, choose a logic-level MOSFET rated for low Rds(on) at 2.5–4.5 V or add a gate driver.

- Thermal sizing: heater resistance measured ~0.45–0.65 Ω. With battery voltages in the 3.7–4.2 V range, approximate heater currents are:

	- At 3.7 V: I = 3.7 / Rheater → 3.7 / 0.65 ≈ 5.7 A up to 3.7 / 0.45 ≈ 8.2 A
	- At 4.2 V: I = 4.2 / Rheater → 4.2 / 0.65 ≈ 6.5 A up to 4.2 / 0.45 ≈ 9.3 A

	The MOSFET dissipation is approximately Pd ≈ I^2 × Rds(on). Example dissipation numbers (illustrative — replace Rds(on) with datasheet value):

	- If Rds(on) = 10 mΩ: Pd ≈ 6^2×0.01 = 0.36 W (at 6 A); Pd ≈ 9^2×0.01 = 0.81 W (at 9 A)
	- If Rds(on) = 20 mΩ: Pd ≈ 6^2×0.02 = 0.72 W (at 6 A); Pd ≈ 9^2×0.02 = 1.62 W (at 9 A)

	These examples show MOSFET Pd can range from sub‑watt to multiple watts depending on Rds(on) — check the RLB8721PBF datasheet for exact Rds(on) at VGS=3.3 V and use Pd = I^2×Rds(on) to compute expected dissipation.

- Mechanical/thermal guidance: TO-220 allows fitting a small heatsink or mounting to the board with a standoff; if you prefer an SMD solution, pick a MOSFET with similar or lower Rds(on) and design a wide copper pour + thermal vias to move heat to the other side. Consider 2 oz copper for heater nets and MOSFET area to reduce temperature rise.

- Action item: confirm RLB8721PBF Rds(on)@VGS (3.3 V) — once confirmed, update this section with the numeric Pd and recommended heatsink/trace sizing. If Rds(on) is too high at 3.3 V, pick an alternative logic-level MOSFET (e.g., parts specified with Rds(on) at VGS = 2.5–4.5 V) or add a small MOSFET gate driver.

## Datasheet-derived calculations and thermal check (IRLB8721PbF / RLB8721 family)

Datasheet notes (from IRLB8721 family):
- RDS(on) specified: 8.7 mΩ (typ/max figure shown at VGS = 10 V in the datasheet excerpt you provided).
- Thermal: RθJA (Junction-to-Ambient) ≈ 62 °C/W (typical, free-air, TO-220 without heatsink); RθJC ≈ 2.3 °C/W.

Because the board's MCU drives the gate at ~3.3 V, and because any source shunt (R5) raises the MOSFET source under load, you must use Rds(on) at the actual VGS to compute dissipation. The datasheet value of 8.7 mΩ is given at VGS = 10 V and is therefore optimistic for a 3.3 V gate drive; Rds(on) at 3.3 V will be higher (datasheet may not explicitly specify it). Use the following worked examples to estimate worst/best-case dissipation and thermal rise.

Assumptions for examples:
- Heater resistance range: 0.45 Ω – 0.65 Ω
- Battery voltage range for drive: 3.7 V – 4.2 V
- Example currents (approximate):
	- 3.7 V / 0.65 Ω ≈ 5.7 A
	- 3.7 V / 0.45 Ω ≈ 8.2 A
	- 4.2 V / 0.65 Ω ≈ 6.5 A
	- 4.2 V / 0.45 Ω ≈ 9.3 A

MOSFET dissipation Pd ≈ I^2 × Rds(on)

Compute Pd for three Rds(on) scenarios to show sensitivity:

- Rds(on) = 8.7 mΩ (datasheet value at VGS = 10 V)
	- Pd @ 6 A ≈ 6^2 × 0.0087 = 0.31 W
	- Pd @ 9 A ≈ 9^2 × 0.0087 = 0.71 W

- Rds(on) = 20 mΩ (plausible mid-case at low gate drive)
	- Pd @ 6 A ≈ 6^2 × 0.02 = 0.72 W
	- Pd @ 9 A ≈ 9^2 × 0.02 = 1.62 W

- Rds(on) = 40 mΩ (worst-case if the MOSFET is not driven to low Rds at VGS≈3 V)
	- Pd @ 6 A ≈ 6^2 × 0.04 = 1.44 W
	- Pd @ 9 A ≈ 9^2 × 0.04 = 3.24 W

Thermal rise (approximate, free-air TO-220 without heatsink): ΔTj ≈ Pd × RθJA (RθJA ≈ 62 °C/W)

- Using Rds(on) = 20 mΩ and I = 9 A: Pd ≈ 1.62 W → ΔTj ≈ 1.62 × 62 ≈ 100 °C rise above ambient (unsafe without heatsink)
- Using Rds(on) = 8.7 mΩ and I = 9 A: Pd ≈ 0.71 W → ΔTj ≈ 0.71 × 62 ≈ 44 °C rise (may be acceptable at moderate ambient)

Practical consequences and guidance:

- Gate voltage derating due to source sense resistor: if R5 is a sense resistor placed at the MOSFET source, the source voltage will rise with current (Vsource ≈ I × R5), reducing VGS by that amount. Example: R5 = 50 mΩ at I = 9 A gives Vsource ≈ 0.45 V → VGS_actual ≈ 3.3 − 0.45 = 2.85 V. That reduction can measurably increase Rds(on).

- If you plan to run near the high end of heater currents (≈ 8–9 A), do one of the following:
	1. Use a MOSFET that guarantees low Rds(on) at VGS = 2.5–4 V (explicitly specified in the datasheet). Use the datasheet figure at the closest gate voltage to compute Pd precisely.
	2. Keep the sense resistor (R5) very small (< 10–20 mΩ) so Vsource rise is minimal, or move the sense resistor after the MOSFET (if measurement topology allows) to preserve VGS.
	3. Use the TO‑220 and a small heatsink or an insulated standoff to keep junction temperature in a safe range if Pd is expected > 1 W.
	4. Alternatively pick an SMD MOSFET with ultra-low Rds(on) and design a heavy copper area with thermal vias (2 oz copper recommended) to dissipate heat.

- Use a gate pulldown (100 kΩ recommended) to ensure Q2 is off during MCU reset or if the gate is tri‑stated.

Action items (concrete):
### Datasheet excerpt and computed Pd (your screenshot)

The datasheet table you provided contains the following RDS(on) entries:
- RDS(on) = 6.5 mΩ (typ) / 8.7 mΩ (max) @ VGS = 10 V, ID = 31 A
- RDS(on) = 13.1 mΩ (typ) / 16 mΩ (max) @ VGS = 4.5 V, ID = 25 A

Note: the datasheet does not list RDS(on) at 3.3 V. Below are computed MOSFET dissipations (Pd = I^2 × RDS(on)) and approximate junction temperature rises (ΔTj = Pd × RθJA using RθJA ≈ 62 °C/W from the datasheet) for the heater current range. Currents used (approx): 5.7 A, 6.5 A, 8.2 A, 9.3 A (derived from battery 3.7–4.2 V and Rheater = 0.45–0.65 Ω).

Computed Pd and ΔTj (worked examples):

- Using RDS(on) = 8.7 mΩ (datasheet @ 10 V)
	- Pd @ 5.7 A = 0.28 W → ΔTj ≈ 17.5 °C
	- Pd @ 6.5 A = 0.37 W → ΔTj ≈ 22.8 °C
	- Pd @ 8.2 A = 0.59 W → ΔTj ≈ 36.4 °C
	- Pd @ 9.3 A = 0.75 W → ΔTj ≈ 46.7 °C

- Using RDS(on) = 13.1 mΩ (datasheet typ @ 4.5 V)
	- Pd @ 5.7 A = 0.43 W → ΔTj ≈ 26.4 °C
	- Pd @ 6.5 A = 0.55 W → ΔTj ≈ 34.3 °C
	- Pd @ 8.2 A = 0.88 W → ΔTj ≈ 54.6 °C
	- Pd @ 9.3 A = 1.13 W → ΔTj ≈ 70.2 °C

- Using RDS(on) = 16 mΩ (datasheet max @ 4.5 V)
	- Pd @ 5.7 A = 0.52 W → ΔTj ≈ 32.2 °C
	- Pd @ 6.5 A = 0.68 W → ΔTj ≈ 41.9 °C
	- Pd @ 8.2 A = 1.08 W → ΔTj ≈ 66.7 °C
	- Pd @ 9.3 A = 1.38 W → ΔTj ≈ 85.8 °C

- Estimated conservative cases for VGS ≈ 3.0–3.3 V (estimate — datasheet not provided):
	- If RDS(on) ≈ 25 mΩ:
		- Pd @ 5.7 A ≈ 0.81 W → ΔTj ≈ 50.4 °C
		- Pd @ 6.5 A ≈ 1.06 W → ΔTj ≈ 65.5 °C
		- Pd @ 8.2 A ≈ 1.68 W → ΔTj ≈ 104.2 °C
		- Pd @ 9.3 A ≈ 2.16 W → ΔTj ≈ 134.1 °C
	- If RDS(on) ≈ 30 mΩ:
		- Pd @ 5.7 A ≈ 0.97 W → ΔTj ≈ 60.4 °C
		- Pd @ 6.5 A ≈ 1.27 W → ΔTj ≈ 78.6 °C
		- Pd @ 8.2 A ≈ 2.02 W → ΔTj ≈ 125.1 °C
		- Pd @ 9.3 A ≈ 2.59 W → ΔTj ≈ 160.9 °C

Interpretation:
- The IRLB8721 family has excellent RDS(on) at 10 V and good values at 4.5 V (13.1–16 mΩ). At MCU gate voltages (~3.3 V) the on‑resistance will be higher; without a guaranteed RDS(on) at 3.3 V the safe course is to assume RDS(on) in the ~20–30 mΩ range for worst-case planning. At those values and currents you may need a heatsink or an SMD MOSFET with heavy copper to keep junction temperature within safe limits.

Recommendations (concrete):
- If you keep RLB/IRLB8721: ensure the MOSFET is mounted to a heatsink or exposed copper (TO‑220 tab) for expected Pd above ~1 W. Use the TO‑220 mounting hole and small heatsink or a thermal pad + insulator.
- If you need a low-profile PCB solution: pick an SMD MOSFET that guarantees RDS(on) at VGS ≤ 4 V (ideally specified at 2.5–3.3 V) and design a wide copper area + thermal vias. Consider upgrading heater net copper to 2 oz if you keep high current on PCB traces.
- Reduce or eliminate the source sense resistor (R5) or keep it very low (< 10–20 mΩ) to avoid Vsource rise that reduces VGS and increases RDS(on).
- Add a gate pulldown (100 kΩ) and keep the gate series resistor (R4) around 10–100 Ω to tame ringing.

If you want, I will:

- Add the computed numeric table above into the BOM and add recommended heatsink/copper area sizes (I can calculate approximate heatsink Rθ and size for a target ΔT). (say: "add heatsink guidance")

- Search and propose 2–3 alternative MOSFET parts explicitly specified at VGS = 2.5–4 V and add them to the BOM as alternates. (say: "suggest alternates")

Below are concrete heatsink and PCB thermal recommendations I will add to the docs/BOM now (and have added when you asked for 'first and third options'):

### Heatsink sizing quick-reference

- Use Tj_max = 125 °C and pick a safe ambient (e.g., 25 °C). Choose a working ΔT_target = 40–60 °C depending on how conservative you want to be.
- For any expected Pd, compute RθJA_total ≤ ΔT_target / Pd. With RθJC ≈ 2.3 °C/W for TO-220, the allowed case‑to‑ambient RθCA (heatsink+interface) is RθCA ≤ RθJA_total − 2.3.

Examples (rounded):
- Pd = 1.1 W → for ΔT_target = 40 °C: RθJA_total ≤ 36 °C/W → RθCA ≤ 34 °C/W. A small clip‑on heatsink rated 20–30 °C/W is sufficient.
- Pd = 2.2 W → for ΔT_target = 40 °C: RθJA_total ≤ 18 °C/W → RθCA ≤ 16 °C/W. Use a moderate heatsink (≤ 15 °C/W) or a chassis mounting.
- Pd = 2.2 W → for ΔT_target = 60 °C: RθJA_total ≤ 27 °C/W → RθCA ≤ 25 °C/W — small heatsink acceptable.

PCB option (no external heatsink):
- Use an SMD MOSFET specified at low VGS (≤ 4 V). Provide a large copper pad (several cm²) with 8–16 thermal vias into a bottom copper plane; prefer 2 oz copper in the MOSFET area. This is suitable for Pd up to ~1–2 W depending on area and via count (ask your board house for thermal guidance for your exact layout).

Recommendation summary:
- If you expect frequent peaks near 9 A, prefer TO‑220 with a heatsink or a very low‑Rds(on) SMD MOSFET with heavy copper + vias. If you expect short pulses only, a smaller heatsink or heavier copper plane may suffice.

### Alternate MOSFET selection (search filters & example families)

I will not pick exact SKUs for you unless you want me to fetch vendor SKUs, but here are concrete search filters and example families to choose from. Use these filters on DigiKey/Mouser and select parts with datasheets listing Rds(on) at VGS ≤ 4 V (ideally 2.5–3.3 V):

- Filters to set on distributor sites:
	- Rds(on) ≤ 20 mΩ (at VGS ≤ 4.5 V)
	- VDS ≥ 30 V (battery headroom)
	- Continuous drain current ≥ 20 A (package thermal allowance)
	- Package: TO‑220 (if heatsink) or DPAK/TO‑252, SO‑8/SON for SMD options

- Example reputable families to consider:
	- Infineon OptiMOS / IRL-series (look for parts explicitly characterized at 4 V or lower)
	- Vishay SiR (SuperSO8 / PowerPAK families) — many parts specify low Rds(on) at 4 V
	- Nexperia / NXP PSMN series (SMD power MOSFETs with good low‑VGS options)

If you want exact SKUs and price/stock, say "fetch alternates" and I'll query DigiKey/Mouser and add 2–3 candidate SKUs (with footprint, datasheet link, and price) into a new BOM CSV file.

If you want me to add the heatsink guidance and alternates to the BOM now, say "add heatsink guidance" or "suggest alternates" respectively and I'll create/update the BOM CSV.

1. From the IRLB8721/RLB8721 datasheet, find Rds(on) at the lowest VGS specified (if available). If the datasheet does not include a 3.3 V figure, pick the Rds(on) at 4 V (or use the transfer curve) or choose a MOSFET that explicitly specifies Rds(on) at 2.5–4 V.
2. Decide whether R5 is required as a sense resistor — if yes, pick a low-value shunt (10–50 mΩ) and document it in the BOM; if not, tie the MOSFET source directly to BAT‑.
3. If expected Pd (from I^2·Rds) > ~1.5 W in free-air, plan for a heatsink or change to an SMD solution with thermal vias and wider copper.

I can update these numeric values into the BOM/notes (done) and, if you give me the exact Rds(on) at your expected VGS (or allow me to fetch the full datasheet), I will compute precise Pd and recommended heatsink specs (size/thermal resistance) and update the docs accordingly.

5) Mechanical, keepouts, and silkscreen
- Add or verify mechanical keepouts and `rule_area` zones around connectors, the heater area, and any areas reserved for mounting.
- Add silk assembly markings (polarity, pin-1 arrows, connector labels). Ensure silkscreen does not overlap pads.
- Add 2–3 fiducials on the PCB (top-left, bottom-right) away from populated areas.
- Confirm mounting hole sizes and add non-plated if necessary.

6) DRC and design-rule tuning
- Set the DRC to the fab's minimums (clearance, min annular ring, min via-to-pad, etc.).
- Run DRC. Investigate any violations; common fixes: move silks, increase annular ring, add thermal reliefs, change pad sizes.

7) Thermal and copper pour
- For copper pours/polygons, set proper isolation and thermal relief for through-hole pads that need it.
- Pour the zones and check connectivity and thermal reliefs.

8) Footprint courtyards and assembly clearance
- Verify courtyard clearances for all footprints (especially the XIAO module, connectors, and encoders). Keep adequate space for pick-and-place nozzle and manual soldering.

9) BOM finalization
- Replace generic part entries with real manufacturer/distributor PNs and preferred footprints. Example suppliers: LCSC (cheapest), Mouser, Digi-Key. For through-hole 100k resistors you can pick commonly stocked axial resistor PNs (e.g., YAGEO/Vishay 100k 1% axial) or prefer SMD if assembler requires.
- Note any parts to be hand-soldered or installed later (e.g., battery connectors if you want them DNP).
- Lock quantities and note alternates.

10) Stencil/paste and assembly notes (for SMT)
- If you have SMT parts, generate stencil drawings and add paste-coverage notes. For through-hole parts plotted for wave soldering, ensure their pads/annular rings are appropriate.

11) Generate manufacturing outputs
- Gerbers (RS-274X with embedded apertures) for each layer: F.Cu, B.Cu, F.Silk, B.Silk, F.Mask, B.Mask, Edge.Cuts
- Drill (Excellon) file
- NC Drill with proper tool sizes
- Pick-and-place (XY) CSV including reference, value, footprint, rotation, X, Y (in mm) — KiCad plot options provide this
- BOM CSV with MPNs and footprint names
- Assembly drawing: PDF with top/bottom placement and notes
- IPC netlist (optional) for assembly/test

12) DFM/DFT review
- Upload Gerbers to the intended fab for DFM feedback or use online Gerber viewers and DFM tools.
- Consider testability: add testpoints for critical nets or program/test pads for production programming.

13) Prototype and test
- Order 5–10 prototypes first (bare and assembled if you want). Verify that footprints and tolerances match expectations.
- Create a simple test checklist: power rails OK, heater continuity, MOSFET gate logic, ADC reading from battery divider, thermocouple reading.

14) Panelization and final notes
- If building at scale, panelize boards with rails and fiducials; include breakaway tabs, tooling holes, and board orientation.
- Freeze files on sign-off and document the version (e.g., `ESPAtomizer_v0.2_release`), timestamp, and who approved.

## Suggested DRC & fab parameters to confirm with your PCB house
- Min track/space: 6 mil (0.15 mm) or better if your fab supports 4 mil.
- Min annular ring: 6 mil.
- Min via drill: 0.3 mm.
- Copper weight: 1 oz (or 2 oz for heater nets).
- Surface finish: ENIG for best lifecycle; HASL for prototypes.

## Edge cases & cautions
- Axial through-hole resistors are mechanically large; they can conflict with connectors and the heater element. If board real estate is tight, consider SMD + manual rework or placing axial resistors off-board.
- Make sure the battery sense net is high impedance and routed away from noisy digital signals (keep trace length short to ADC pin).
- If designing for LiPo battery operation, consider adding a fuse (polyfuse) and reverse-polarity protection (diode or MOSFET) in hardware.

## Deliverables (what to give the manufacturer)
- Gerber & drill files (ZIP)
- README/fab notes (board thickness, copper weight, finish, tolerance)
- BOM CSV with MPNs and manufacturer links
- Pick-and-place CSV
- Assembly drawing PDF
- Stencil file (if SMT assembly is requested)
- Panelization drawing (if relevant)

## Quick checklist you can follow now (short)
1. Wire R4 and R5 in schematic; run ERC.
2. Verify footprints and update PCB from schematic.
3. Place components, add fiducials, and set net classes for heater.
4. Run DRC and fix violations.
5. Finalize BOM with MPNs; produce pick-and-place and Gerbers.
6. Upload to fab for DFM review and prototype order.

---

If you want, I can:
- Fill in a vendor-specific BOM (Mouser/DigiKey/LCSC) — tell me your preferred supplier.
- Programmatically add footprints to `ESPAtomizer.kicad_pcb` and connect nets (I can do this but it's higher-risk; I recommend manual placement for through-hole axial parts).
- Create a test jig spec and a simple test script for the firmware to exercise heater and ADC during production testing.

Tell me which of the above you'd like me to do next and I'll update the todo list and proceed.
