ESPAtomizer Bring-up Checklist

This checklist helps verify basic hardware and firmware functionality after flashing.

Prerequisites
- USB serial connected to device. Use 230400 baud by default (see `config.h`).
- Power: LiPo connected or USB providing power.
- Serial monitor (Arduino IDE, PlatformIO serial monitor, or `picocom`/`screen`).

Basic steps

1) Boot & Serial
- Open serial monitor at `230400` baud, 8N1.
- Reboot device and observe boot logs. Confirm `Setup complete!` message.

2) Pin mapping sanity
- Confirm printed `Pins:` line on boot shows expected GPIO numbers.

3) OLED
- If you have an OLED connected, watch for `"[OLED] Init starting..."` and either a success or failure message.
- Serial commands:
  - `Z` toggles serial streaming.
  - `P` prints a snapshot (temp/setpoint/out).

4) Encoder
- Test click and rotation:
  - Short press of encoder should toggle power or open/close menu (long press opens menu).
  - Serial commands:
    - `E` prints encoder diagnostics
    - `EZ` zeroes encoder counters

5) MAX6675 thermocouple
- Ensure thermocouple is plugged into J4.
- Serial command `T` prints raw thermocouple reading and connection status.
- Expected: either a temperature in C or `NaN` if open.

6) Battery monitor
- Serial command `B` prints battery snapshot.
- `BC` runs a short charge check sequence (default 20s).

7) Output / MOSFET
- Serial command `XT` toggles the output for 1s (X1/X0/XT commands exist).
- Verify heater MOSFET toggles (listen for clicks, or measure output node with meter).

8) BLE (if enabled)
- Ensure advertising started message on boot from BLE subsystem.
- Use a phone/central to scan and connect; observe bonding/pairing behavior if enabled.

Common debugging tips
- If OLED not found: check SDA/SCL wiring and try swapping.
- If MAX6675 reads `NaN`: verify Vcc/GND and SO/SCK/CS wiring; try different MAX6675 library variants.
- If encoder behaves inverted: check `ENC_DIR` in `config.h`.

Building with pin overrides
- PlatformIO: `build_flags = -DOUTPUT_PIN=5 -DD10_PIN=19`
- Arduino CLI: `arduino-cli compile --fqbn <your_fqbn> -DOUTPUT_PIN=5 -DD10_PIN=19 path/to/ESPAtomizer`

If any test fails, capture the serial log and report it for further diagnosis.
