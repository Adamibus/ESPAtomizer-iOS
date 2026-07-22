# ESPAtomizer

ESPAtomizer is a firmware + hardware project for a compact, battery-powered atomizer using ESP32 (XIAO ESP32-C3/C6). The repo includes:
- Firmware (Arduino-based) in `ESPAtomizer/`
- iOS companion app in `ESPAtomizer-iOS/`
- Hardware design files in `hardware/`
- Docs and bring-up notes in `docs/`
- Developer tools in `tools/`

## Software-Only Testing
No board yet? You can still validate structure and logic.

- Tools overview: see [tools/README.md](tools/README.md)
- Smoke check (sketch structure + test scaffolding):
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "c:\Users\Adam Dinjian\OneDrive\Projects\Coding\ESPAtomizer\tools\smoke_check.ps1"
```
- Config lint (parse and validate `ESPAtomizer/config.h` constraints):
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "c:\Users\Adam Dinjian\OneDrive\Projects\Coding\ESPAtomizer\tools\config_lint.ps1"
```
- PID simulator (exercise PID behavior without hardware):
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "c:\Users\Adam Dinjian\OneDrive\Projects\Coding\ESPAtomizer\tools\pid_sim.ps1" -Setpoint 180 -Kp 8 -Ki 0.3 -Kd 40 -DurationMs 15000 -StepMs 100 -Ambient 22 -PwmMax 1023
```

### TEST_MODE (firmware logic stubs)
Enable compile-time test mode to exercise logic without hardware I/O:
- File: `ESPAtomizer/config.h`
- Set `#define TEST_MODE 1`
- Effects:
  - `ESPAtomizer/ESPAtomizer.ino`: `readTemperatureC()` returns a simulated temperature; `applyOutput()` logs duty without PWM
  - `ESPAtomizer/battery.h`: `sampleBattery()` returns simulated voltage/percent

## Firmware Quick Start
- IDE: Arduino IDE (ESP32 core)
- Board: Seeed XIAO ESP32-C3 or ESP32-C6
- Sketch: `ESPAtomizer/ESPAtomizer.ino`
- Serial: 230400 baud (see `SERIAL_BAUD` in `ESPAtomizer/config.h`)
- BLE: NimBLE via ESP32 Arduino core; no separate library required

## Useful Docs
- Bring-up and ESP32-C6 notes: [docs/README-ESP32C6.md](docs/README-ESP32C6.md)
- Boot self-tests: [docs/BOOT-SELFTEST-VERIFICATION.md](docs/BOOT-SELFTEST-VERIFICATION.md)
- Hardware: see `hardware/ESPAtomizer_PCB/`

## License
Proprietary project files. Hardware and app directories may include additional notices.
