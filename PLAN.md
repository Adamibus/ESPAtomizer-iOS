# ESPAtomizer

Firmware + iOS app + hardware for a battery-powered dab-rig atomizer on a Seeed
**XIAO ESP32-C3 / C6**. The iOS app controls the device over Bluetooth LE.

```
ESPAtomizer/
├── ESPAtomizer/         firmware (Arduino sketch + managers)   → config.h has all flags/pins
├── ESPAtomizer-iOS/     iOS app (Xcode target "ESPAtomizer", source in ESPAtomizer/)
├── hardware/            KiCad PCB + gerbers
├── docs/                bring-up notes, BLE protocol, audits
└── tools/               dev helpers
```

---

## Build & run

### Firmware (Arduino IDE 2.x)
1. Boards Manager → install **esp32 by Espressif** (v3.x). Board: **XIAO ESP32-C6** (or C3).
2. Libraries: **NimBLE-Arduino** (h2zero), **PID** (`PID_v1`), **Adafruit SSD1306** + **Adafruit GFX**, **Adafruit ADS1X15**.
   > ⚠️ NimBLE-Arduino is required — `NimBLEDevice.h` is *not* bundled with the ESP32 core, despite what `docs/README.md` says.
3. Open `ESPAtomizer/ESPAtomizer.ino`, flash, Serial @ **230400 baud**. BLE is on by default;
   you should see `[BLEManager] BLE initialized` and `"Adamizer"` advertising service `b09aa6b5-…`.
   - No hardware yet? Build with `-DTEST_MODE=1` to stub sensors/PWM and test logic on the bench.

### iOS app (Xcode)
1. Open `ESPAtomizer-iOS/ESPAtomizer-iOS.xcodeproj`, set your Signing team (target **ESPAtomizer**).
2. Run on a **physical iPhone** (BLE doesn't work in the Simulator). Approve the pairing prompt.

Hardware bring-up: see `docs/BRINGUP.md` + `docs/README-ESP32C6.md`. Key pins: heater PWM = GPIO16,
encoder A/B/SW = D1/D2/D3, OLED + ADS1115 on I²C, battery sense on A0. Thermal safety trips at
320 °C / 50 °C-over-setpoint — keep `ENABLE_THERMAL_SAFETY=1`.

---

## Roadmap to a final optimized version

Ordered path to a polished v1.0. Each phase has a **Done when** acceptance gate.

**1 · Baseline — verify on real hardware.** Flash the firmware and run the app per
[Build & run](#build--run). This is the one phase that needs physical hardware and hasn't been
run yet. Before flashing, run the contract check: `python3 tools/ble_contract_check.py`.
*Done when:* the app pairs silently, shows live temperature, mode switching takes effect, battery
shows a real value, a rejected write shows an alert, and a disconnected thermocouple shows the
fault banner.

**2 · Field-readiness & release.** Add OTA (enable the stubbed WiFi/`ArduinoOTA` path, or
OTA-over-BLE, with version negotiation) plus basic observability. Validate heater thermal safety on
hardware **before any unattended run**. Then set a reverse-DNS bundle id + signing, consolidate git
(the app is still a nested repo — its own `.git` under `ESPAtomizer-iOS/`, gitignored by the parent;
fold it in or keep separate), and archive a Release build.
*Done when:* firmware updates without USB, a Release archive builds, and the repo is one tree.

**Definition of done:** pairs silently · every control two-way and acknowledged · history
persists · versioned + tested BLE contract · themed, consistently-branded UI · OTA-updatable ·
thermal safety verified on hardware.

---

## Docs
`docs/BLE-PROTOCOL.md` (characteristic contract) · `docs/FIRMWARE-BLE-AUDIT.md` /
`docs/README-FIRMWARE-FIXES.md` (firmware notes) · `docs/BRINGUP.md`, `docs/README-ESP32C6.md`
(hardware) · `docs/README.md` (original overview + test tooling).
