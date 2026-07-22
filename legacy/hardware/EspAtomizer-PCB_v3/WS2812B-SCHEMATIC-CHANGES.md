# WS2812B RGB LED Schematic Update Checklist

**Objective:** Migrate from discrete RGB (GPIO1/2/21) to WS2812B addressable LED on GPIO17  
**Date:** December 26, 2025

---

## Schematic Changes (EspAtomizer-PCB_v3.kicad_sch)

### Remove Discrete RGB Components

- [ ] Delete or disconnect `LEDR` net label (was GPIO1)
- [ ] Delete or disconnect `LEDG` net label (was GPIO17 — this can stay but will be repurposed)
- [ ] Delete or disconnect `LEDB` net label (was GPIO21)
- [ ] Remove resistors R_RED, R_GREEN, R_BLUE (220Ω current-limiting resistors, if present)
- [ ] Remove discrete RGB LED footprint/symbol from the schematic
- [ ] Remove any associated pull-down or bias resistors

### Add WS2812B Module

- [ ] Add WS2812B LED component (or discrete WS2812B IC like WS2812B-2020) to schematic
  - **Pins needed:** DIN (data in), GND, VDD (5V or 3.3V depending on module)
  - Common footprints: 0603 (discrete), 4-pin module, or pre-assembled ring/strip

- [ ] Connect WS2812B DIN → **GPIO17 (U3 pin 8 / D7 / `GPIO17_D7_RX`)** with net label **`RGB_LED_DATA`** (or `WS2812B_DATA`)
  - Optional: Add 470Ω series resistor on data line for signal integrity

- [ ] Connect WS2812B VDD → **3.3V (VCC rail)** 
  - Add 100nF bypass capacitor between VDD and GND near the LED

- [ ] Connect WS2812B GND → **GND (U3 pin 13 or 20)**

### Verify I2C and Power Nets

- [ ] Confirm `I2C_SDA` and `I2C_SCL` labels are on U3 pins 5 & 6 (GPIO22/23) — unchanged
- [ ] Confirm `BAT_ADC` label is on U3 pin 1 (GPIO0) — unchanged
- [ ] Confirm encoder nets (`ENC_A`, `ENC_B`, `ENC_SW`) are on U3 pins 2, 3, 4 (GPIO1/2/21) — unchanged
- [ ] Confirm `MOSFET_GATE` label is on U3 pin 7 (GPIO16) — unchanged

### Clean Up Silkscreen

- [ ] Remove or update silkscreen label for RGB LED (if present)
- [ ] Add silkscreen label near WS2812B: **"RGB LED (WS2812B)"** or **"STATUS_LED"**
- [ ] Verify GPIO17 pad on U3 is clearly visible/not covered

---

## PCB Layout Changes (EspAtomizer-PCB_v3.kicad_pcb)

### Update Netlist and Re-route

- [ ] Run **Tools → Update PCB from Schematic** in KiCad to sync netlist
- [ ] Delete old discrete RGB footprints from the PCB layout
- [ ] Place WS2812B module/LED near the XIAO module or enclosure cutout
- [ ] Route GPIO17 (U3 pin 8) → WS2812B DIN with short trace (< 10 cm preferred)
- [ ] Route 3.3V → WS2812B VDD with wide trace (minimize voltage drop)
- [ ] Route GND → WS2812B GND with solid ground connection

### Add Decoupling

- [ ] Place 100nF (0.1µF) bypass capacitor between WS2812B VDD and GND
  - Position capacitor pads within 5mm of the LED pads for maximum effectiveness
  - Use 0603 or 0402 SMD footprint for compact placement

### Optional: Signal Integrity

- [ ] Add 470Ω series resistor on GPIO17 → DIN line (optional but recommended for > 5cm traces)
  - Protects against data line reflections and ringing
  - Minimal impact on signal timing

---

## Firmware Verification (Already Done)

- [x] config.h updated: `RGB_LED_PIN = 17`, `RGB_LED_ENABLED = 1`, `RGB_LED_COUNT = 1`
- [x] Conflicting `RGB_PIN_RED/GREEN/BLUE` defines removed
- [ ] Firmware code updated to use NeoPixel or Adafruit_NeoPixel library for WS2812B control
- [ ] Compile firmware for both `xiao_esp32c3` and `xiao_esp32c6` to verify no conflicts

---

## Cross-Board Compatibility

- [x] GPIO17 available on both XIAO ESP32-C3 and ESP32-C6 ✅
- [x] 3.3V rail available on both boards ✅
- [x] No strap/boot pin conflicts ✅
- [x] No interference with I2C, encoder, battery, or MOSFET pins ✅

---

## Validation Checklist

- [ ] Run KiCad **Design Rules Check (DRC)** — should pass with no unconnected nets
- [ ] Run KiCad **Electrical Rules Check (ERC)** — should flag no new errors
- [ ] Generate **netlist** and verify GPIO17 → `RGB_LED_DATA` binding to U3 pin 8
- [ ] Visually inspect schematic: no orphaned nets or floating pins
- [ ] Visually inspect PCB: WS2812B placement, trace routing, bypass cap placement

---

## Test After Assembly

- [ ] Upload firmware with WS2812B initialization code
- [ ] Verify RGB LED responds to color commands
- [ ] Test color transitions and PWM brightness
- [ ] Confirm no I2C/encoder/PWM/ADC interference

---

**Status:** Schematic changes ready for implementation. Once completed, run ERC/DRC and notify for next steps.
