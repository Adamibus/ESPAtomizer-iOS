# ESPAtomizer

A compact, battery-powered atomizer controller built on the Seeed XIAO ESP32-C6 —
closed-loop temperature control with a K-type thermocouple, OLED + rotary encoder
interface, and a SwiftUI companion app over BLE.

**Status:** active prototype · firmware in bring-up

## Safety

This device drives a heating element.

- **Default setpoint:** 240 °C
- **Thermal cutoff:** TODO
- **Sensor fault behavior:** TODO — thermocouple open-circuit
- **Battery:** TODO — chemistry, charge circuit, protection

Never run the heater unattended, and never bypass `SafetyManager`.
Pack limits: [`docs/battery-pack.md`](docs/battery-pack.md).

## Repo layout

| Path | Contents |
|---|---|
| [`firmware/`](firmware/) | Arduino firmware for the ESP32-C6 |
| [`ios-app/`](ios-app/) | SwiftUI companion app |
| [`hardware/`](hardware/) | KiCad sources, gerbers, board notes |
| [`hardware/mechanical/`](hardware/mechanical/) | Enclosure and stand CAD |
| [`docs/`](docs/) | Bring-up guides, protocol specs, hardware reviews |
| [`tools/`](tools/) | Static checks and analysis scripts |
| [`legacy/`](legacy/) | Pre-rewrite project, frozen for reference |

## Getting started

### Firmware

Open `firmware/ESPAtomizer.ino` in the Arduino IDE with the ESP32 board package.
Target board: **Seeed XIAO ESP32-C6**.
Pinout and wiring: [`docs/README-ESP32C6.md`](docs/README-ESP32C6.md).

```bash
python tools/ble_contract_check.py    # verify BLE characteristics, no hardware needed
```

### iOS app

Open `ios-app/ESPAtomizer-iOS.xcodeproj` in Xcode 15+.
BLE requires a physical device — the simulator has no Bluetooth radio.
Set your signing team under Signing & Capabilities.

### Hardware

Connector pinouts:
[`hardware/ESPAtomizer_PCB/CONNECTORS.md`](hardware/ESPAtomizer_PCB/CONNECTORS.md).

## BLE protocol

Wire format: [`docs/BLE-PROTOCOL.md`](docs/BLE-PROTOCOL.md).
Update it in the same commit as any characteristic change — firmware and app
ship independently and will silently desync otherwise.

## Contributing

Branch from `main`, open a pull request. See [`CONTRIBUTING.md`](CONTRIBUTING.md).

## License

TODO
