# XIAO ESP32-C3 vs ESP32-C6 Compatibility — Final Check

Date: December 26, 2025

Purpose: Final verification and concise guidance to ensure the Seeed XIAO ESP32-C3 and XIAO ESP32-C6 dev modules are interchangeable in this design.

Summary
- The XIAO ESP32-C3 and XIAO ESP32-C6 share the same 2×7 header footprint; this project uses the standard 14-pin header only.
- Firmware uses D-pin macros (`D0..D10`) and `SDA`/`SCL` macros for portability — do NOT hardcode GPIO numbers in application code.
- With the current schematic and `config.h` aliases, both modules are compatible without hardware changes.

Key rules (must follow)
1. Build / board selection: Choose the correct Arduino board/variant in your IDE ("Seeed XIAO ESP32-C3" or "Seeed XIAO ESP32-C6"). Alternatively, set `-DBOARD_TYPE=BOARD_XIAO_ESP32_C3` or `-DBOARD_TYPE=BOARD_XIAO_ESP32_C6` at compile time.
2. Always use `A0` for battery ADC (maps to ADC1 on both C3 and C6).
3. Use `SDA` and `SCL` macros for I2C (OLED + ADS1115). The firmware calls `Wire.begin(SDA, SCL)`.
4. Avoid ADC2 pins on ESP32-C3 (see note below).
5. Use `D10` for MOSFET gate PWM (avoids strap pins/boot UART chatter).

Canonical pin aliases used by firmware (see `ESPAtomizer/config.h`)
- LEFT header pads (footprint mapping, canonical GPIOs used in firmware):
  - Pad1 (D0 / A0) → GPIO0
  - Pad2 (D1) → GPIO1
  - Pad3 (D2) → GPIO2
  - Pad4 (D3) → GPIO21
  - Pad5 (D4) → GPIO22 (D4/SDA)
  - Pad6 (D5) → GPIO23 (D5/SCL)
  - Pad7 (D6) → GPIO16
- D-pins further (config.h defaults):
  - `D8_PIN = 19`
  - `D9_PIN = 20`
  - `D10_PIN = 18`

Notes on variant differences and why macros are used
- Underlying GPIO numbers for a labeled Dn pin can differ by board variant. The Arduino cores/variants map the Dn macros to the correct GPIO on each board. Rely on the `D` macros and `SDA`/`SCL` macros to remain portable.
- Example: some code or library comments may reference GPIO19/GPIO20 for I2C on an ESP32-C3 variant; on XIAO-C6 the hardware may route I2C to GPIO22/GPIO23. The project abstracts this via macros.

ADC note
- `BAT_PIN` is defined to `A0` (D0) and maps to ADC1 on both C3 and C6. Avoid using ADC2 channels on the C3 variant due to potential reliability when Wi-Fi is enabled.

Boot/strap & UART notes
- Some D-pins are strap or UART pins on certain variants and can produce activity at reset (e.g., D6 may show UART0 TX chatter). The design currently avoids using strap pins for critical signals:
  - `OUTPUT_PIN` is `D10` to avoid UART chatter and strap effects.
  - Encoder pins default to D1/D2/D3 to avoid boot/strap pins and UART TX.

I2C pull-ups and addresses
- I2C bus is shared between OLED and ADS1115. The schematic includes external pull-ups (recommended 4.7kΩ) or relies on internal pull-ups if documented.
- ADS1115 default I2C address is `0x48` (ADDR tied to GND). See schematic notes for address selection pads.

Build / verification checklist
- [ ] Select proper board variant in IDE or set `-DBOARD_TYPE` macro.
- [ ] Confirm `OLED_SDA`/`OLED_SCL` macros resolve to `SDA`/`SCL` (they are set to `SDA` and `SCL` by default in `config.h`).
- [ ] Compile for both `xiao_esp32c3` and `xiao_esp32c6` in CI or locally to ensure no compile-time alias issues.
- [ ] Run the board pin print test (included example): `examples/ads1115_integration_example.cpp` prints `BAT, OUT, ENC_A, ENC_B, ENC_SW, SDA, SCL` values — verify these match expected Dn labels at runtime.

Quick verification runtime steps
1. Build and flash with `Seeed XIAO ESP32-C3` selected. Open serial at `230400` and verify printed pin numbers and I2C device responses (OLED and ADS1115).
2. Repeat with `Seeed XIAO ESP32-C6` selected. Confirm identical logical behavior (pins may print different GPIO numbers but Dn labels should function the same).

Files touched and references
- `ESPAtomizer/config.h` — canonical alias mapping, board detection, macros
- `hardware/EspAtomizer-PCB_v3/FINAL-PIN-MAPPING-STANDARD-HEADER.md` — schematic pin mapping and notes
- `ESPAtomizer/src/ads1115_driver.cpp` & `ESPAtomizer/ads1115_driver.h` — uses `Wire.begin(SDA, SCL)` (portable)

Conclusion
- With the current codebase and `config.h` aliases, the XIAO ESP32-C3 and XIAO ESP32-C6 are compatible plug-and-play for this design. Use the macros, select the correct board variant while building, and follow the small ADC/strap pin notes above.

If you want, I can:
- Add a short automated sanity test script that compiles and prints pin mappings for both variants (CI-friendly).
- Add the above checklist into `FINAL-PIN-MAPPING-STANDARD-HEADER.md` as a linked section.
