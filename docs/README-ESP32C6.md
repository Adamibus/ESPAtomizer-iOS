# ESP PID Temperature Controller — Seeed Studio XIAO ESP32C6

This wiring guide targets the Seeed Studio XIAO ESP32C6 (Arduino core v3.x). It matches the current sketch at `ESPAtomizer/ESPAtomizer.ino`. The ESP, OLED, and a thermocouple breakout (MAX6675) are intended to be socket‑mounted to save cost and allow easy replacement.

- Sensor: K‑type thermocouple via MAX6675 (SPI‑like) on 3 lines: SCK, CS, SO. 3.3 V logic. Enabled by default (`USE_MAX6675=1`); install a MAX6675 library.
- Output: PWM to MOSFET (heater) at ~1 kHz (disabled by default for safety)
- Controls: Rotary encoder with push switch (Adafruit PID 377 / EC11‑style) for setpoint and mode/power
- Display: Optional SSD1306 I2C OLED (128×64), address 0x3C typical (silk "0x78" = 0x3C 7‑bit)

## Pin Assignments (by GPIO)

The sketch uses explicit GPIO numbers (printed at boot). Wire to these GPIOs, not Dx labels:

| Function              | GPIO   | Type   | Notes |
|-----------------------|--------|--------|-------|
| Encoder A             | GPIO0  | Input  | `ENC_PIN_A` (INPUT_PULLUP) |
| Encoder B             | GPIO1  | Input  | `ENC_PIN_B` (INPUT_PULLUP) |
| Encoder Switch (SW)   | GPIO2  | Input  | `BUTTON_PIN`/`ENC_PIN_SW` (INPUT_PULLUP) |
| OLED SDA              | GPIO22 | I2C    | primary I2C SDA |
| OLED SCL              | GPIO23 | I2C    | primary I2C SCL |
| PWM output (MOSFET)   | GPIO16 | PWM    | `OUTPUT_PIN` (LEDC pin‑API in Arduino 3.x) |
| MAX6675 SCK           | GPIO18 | SPI    | Configurable (D10) |
| MAX6675 CS            | GPIO20 | SPI    | Configurable (D9) |
| MAX6675 SO            | GPIO19 | SPI    | MISO, configurable (D8) |
| Battery sense (opt.)  | GPIO21 | ADC    | `BAT_PIN` via divider (D3), enabled by default |

Board power and rails:
- 3V3 pin → 3.3 V output
- GND → Ground
- 5V pin is USB VBUS; don’t power 3.3 V sensors/displays from 5 V.
 - BAT pad (pad 32 on the XIAO module) is bidirectional: with a LiPo attached it powers the board, and with USB attached the on‑board charger can source current out to the LiPo via BAT. Treat any pogo/test pads tied to BAT as the battery rail (not input‑only). Never tie BAT to 5 V (VBUS).

## Wiring Diagram

The new schematic emphasizes readability and matches the final XIAO ESP32‑C6 pin mapping. It includes the MAX6675, SSD1306 OLED (I2C), rotary encoder (A/B/SW), and the IRLB8721PBF MOSFET output.
Power rails are labeled at module pads (3V3/GND) rather than drawn as traces to reduce clutter.

![ESPAtomizer Wiring — XIAO ESP32‑C6](./wiring-esp32c6-v4.svg)

Legend (colors in the diagram):
- Green = I2C (SDA/SCL)
- Orange = MAX6675 SPI‑like (SCK/CS/SO)
- Blue = Rotary encoder (A/B/SW)
- Red = PWM output to MOSFET gate
- Purple = 3V3, Black = GND

### Quick wiring checklist

Rotary encoder (Adafruit 377 / EC11 with push switch):
- A → GPIO0 (D0)
- B → GPIO1 (D1)
- C (common) → GND
- SW → GPIO2 (D2); the other SW terminal → GND
- No VCC needed (mechanical). Firmware enables internal pull‑ups; optional 100 nF caps from C→A and C→B improve debounce.

### Encoder quick test
- Upload `ESPAtomizer/ESPAtomizer.ino` and open Serial Monitor @ 115200.
- On boot you should see pin printouts: `Pins: TEMP=0(D0), OUT=16(D6), ENC_A=0(D0), ENC_B=1(D1), ENC_SW=2(D2), SDA=22(D4), SCL=23(D5), MAX SCK=18(D10), CS=20(D9), SO=19(D8), BAT=21(D3)`.
- Rotate the encoder: you should see `[ENC] det=±1 => Setpoint=...` messages and the OLED target changing.
- Press/release SW: short press toggles Power; long press toggles PID mode (watch OLED tag [AUTO]/[MAN]). Long press again enters menu with modes: AUTO, MAN, U1, U2, Config, Exit.
- In Config mode (select via menu), encoder adjusts default setpoint (persisted in RTC); short press exits.

### Config Mode
- Accessible via encoder menu (long press SW, select "Config").
- Adjust default setpoint for Auto mode using encoder.
- OLED displays "Config: Default SP: XXX.X C".
- Short press SW to exit; setting is saved to RTC.

### Debug Logging
- Firmware logs BLE connection status, battery, temperature, and mode every 10 seconds (e.g., `[DEBUG] BLE connected: 1, Bat: 3.80V (85%), Temp: 25.0C, Mode: 0`).
- Useful for troubleshooting disconnections or power issues.

OLED (SSD1306 128×64) on I2C:
- VCC → 3.3V
- GND → GND
- SDA → GPIO22
- SCL → GPIO23
- Address: 0x3C typical. Many boards print 8‑bit addresses on silk ("0x78" for write) — this corresponds to 7‑bit 0x3C.
- If bus idles at ~0 V, add 4.7k pull‑ups from SDA→3.3V and SCL→3.3V (some modules omit pull‑ups).

MOSFET output (heater at ~4 V supply):
- Gate → GPIO16 (D6) via 100 Ω resistor
- Gate → 100 kΩ pull‑down to GND
- Source → GND (common ground with ESP and heater supply)
- Drain → heater negative; heater positive → +4 V supply
- Use a Schottky diode for inductive loads (fans): anode to GND, cathode to +4 V
- PWM default: ~200 Hz, 10‑bit; output is disabled in current sketch until we enable PID.

IRLB8721PBF pinout (TO‑220AB): 1 = Gate, 2 = Drain (tab), 3 = Source. The metal tab is connected to Drain.

LED test load (replace heater):
- Keep the MOSFET wiring exactly as above; simply replace the heater with an LED in series with a resistor.
- LED anode (+) → series resistor → +supply (same supply you would use for the heater)
- LED cathode (−) → MOSFET drain (the same pad the heater “−” used)
- MOSFET source → GND (unchanged)
- Gate, pull‑down, and common GND remain the same.

Resistor suggestions (safe, visible brightness):
- 3.3 V supply: 330 Ω to 1 kΩ
- 5.0 V USB/VBUS: 680 Ω to 1.5 kΩ
- 1S Li‑ion (3.7–4.2 V): 470 Ω to 1 kΩ

Tip: If the LED seems very dim, try a lower value within the suggested range (e.g., 330–470 Ω). If it’s too bright or you want to reduce current, increase the resistor (e.g., 1 kΩ).

## Using the board's 5 V (VBUS) to power the load

Yes—on the XIAO ESP32‑C6 the 5 V pin is USB VBUS. You can power your MOSFET‑switched load from this pin with these caveats:

- VBUS is only present when USB is connected.
- Current is limited by the USB source and cable (typically 500 mA on many ports). Don’t exceed your port’s rating.
- The ESP32 GPIO drives the MOSFET gate at 3.3 V. Use a logic‑level MOSFET with low Rds_on at Vgs=2.5–3.3 V (e.g., AO3400A, IRLML6344; large can MOSFETs like IRLZ44N also work but are oversized).
- For heavier loads, use an external 5 V supply and keep grounds common with the ESP (tie GNDs together).

For LEDs on 5 V, choose a suitable series resistor (typical 680 Ω to 1.5 kΩ). For inductive loads (motors, relays), add a flyback diode across the load (e.g., 1N5819) oriented cathode to +5 V, anode to the MOSFET/drain side.

## Using a 1S LiPo as the external load supply

You can power the load from an external 1S LiPo (3.7–4.2 V). The ESP32 stays powered via USB/3V3; the LiPo powers only the load. Critical rule: share ground.

- LiPo + → load +; load − → MOSFET Drain; MOSFET Source → GND
- ESP32 GND ↔ LiPo − (common ground)
- GPIO16 (OUTPUT_PIN) → 100 Ω → Gate; 100 kΩ Gate → GND (pulldown)
- For inductive loads add a flyback diode (cathode to LiPo +, anode to Drain)

Tip for LEDs on LiPo: use a 470 Ω–1 kΩ series resistor (depending on desired brightness).

## Powering the entire device from a 1S LiPo

The XIAO ESP32‑C6 exposes BAT pads for a 1S LiPo. You can power the whole device (ESP32 + peripherals + heater output) from a single 1S pack:

- Connect LiPo to the board’s BAT+ / BAT− pads (see the board silkscreen). The onboard regulator supplies 3.3 V to the ESP32 and I2C devices.
- Heater still uses the LiPo + as its supply; low‑side MOSFET switches the return to GND.
- Keep grounds common (they already are when using the BAT pads).

Caveats:
- Check the XIAO ESP32‑C6 documentation for charging behavior and max current from BAT. If you also connect USB, ensure the power path is supported by the board (many XIAO variants include charge management, but verify your revision).
- Use a hardware battery disconnect switch (SW1) inline with BAT+ so you can fully cut power when OFF; avoid shorts as LiPo packs can source very high currents.
- Use wide copper/wires for the heater path (see PCB notes below).
 - If you use pogo/test pads (TP4/TP5) they are intended as service/debug access points — for permanent battery wiring use the JST‑PH connector (`J3`) on the board edge. Remember BAT is bidirectional: with USB attached the charger circuitry may source current to a LiPo, and with LiPo attached those pads will carry battery current. Do not connect BAT pads to 5 V (VBUS).

## Thermocouple (MAX6675)

Primary temperature sensor is a K‑type thermocouple using a MAX6675 module (3.3 V) for 150–400 °C operation. The MAX6675 uses a simple SPI‑like interface without MOSI.

- Power: VCC=3.3 V, GND=GND
- Signals: SCK, CS, SO (MISO)
- Suggested default GPIOs in firmware: SCK=GPIO18 (D10), CS=GPIO20 (D9), SO=GPIO19 (D8) (changeable via defines)

Firmware:
- Set `#define USE_MAX6675 1` near the top of the sketch.
- Install a MAX6675 library via Arduino Library Manager (e.g., “MAX6675” by Adafruit).

Notes:
- Keep thermocouple leads twisted and away from heater switching traces to reduce noise.
- Many MAX6675 breakout boards label pins as: GND, VCC, SCK, CS, SO. Match that order on the 1×5 socket (J4).


## OLED I2C Address Note
- Many modules mark pads as 0x78 / 0x7A (8‑bit addresses). These map to 7‑bit addresses 0x3C / 0x3D used by Arduino.
- Keep the pad bridged to 0x78 for 0x3C. Our code scans for both 0x3C and 0x3D.

## Pin mapping note (avoid Dx confusion)

The sketch uses explicit GPIO numbers and also prints them at boot, for example:

```
Pins: TEMP=0(D0), OUT=16(D6), ENC_A=0(D0), ENC_B=1(D1), ENC_SW=2(D2), SDA=22(D4), SCL=23(D5), MAX SCK=18(D10), CS=20(D9), SO=19(D8), BAT=21(D3)
```

## Pins summary (Dx ↔ GPIO)

Left side (top→bottom):
- D0 → GPIO0: Encoder A, also TEMP ADC node
- D1 → GPIO1: Encoder B
- D2 → GPIO2: Encoder SW (active‑low)
- D3 → GPIO21: Battery monitor ADC (via divider)
- D4 → GPIO22: I2C SDA (OLED)
- D5 → GPIO23: I2C SCL (OLED)
- D6 → GPIO16: PWM output to MOSFET gate

Right side (top→bottom):
- 5V (VBUS)
- ⏚ GND
- 3V3
- D10 → GPIO18: MAX6675 SCK
- D9  → GPIO20: MAX6675 CS
- D8  → GPIO19: MAX6675 SO (MISO)
- D7  → GPIO17: spare

Use these GPIO numbers when wiring; don't rely on Dx labels, which vary by core/board definitions.

Known‑good GPIOs matching the current sketch:
- `ENC_PIN_A = GPIO0`
- `ENC_PIN_B = GPIO1`
- `ENC_PIN_SW`/`BUTTON_PIN = GPIO2`
- I2C: `SDA=GPIO22`, `SCL=GPIO23`
- `OUTPUT_PIN = GPIO16`
- MAX6675: `SCK=GPIO18 (D10)`, `CS=GPIO20 (D9)`, `SO=GPIO19 (D8)`

## Safety
- Start with heater supply disconnected; verify the MAX6675 reads ~room temp and setpoint changes with the encoder.
- Only then enable PID and PWM output.
- Ensure wiring/connectors are appropriate for expected temperatures and current; a thermal fuse is optional and typically omitted here to minimize cost.

## Wireless control from iPhone (Wi‑Fi web UI)

The firmware exposes a built‑in web UI over a Wi‑Fi SoftAP (no external router required). This works from iOS Safari and can be “Added to Home Screen” for an app‑like experience.

- Power the board; after boot it creates an access point named `Atomizer-XXYY` (XXYY are the last two bytes of MAC).
- Default password: `atomizer123` (change in firmware: `WIFI_AP_PASSWORD`).
- Join the AP from your iPhone, then open `http://192.168.4.1/`.

From the page you can:
- Toggle power ON/OFF
- Switch PID mode AUTO/MAN
- Set setpoint (°C)
- Tune Kp/Ki/Kd
- In MAN mode, set manual output (0..1023)
- Choose setpoint source (POT vs FIXED)

Status updates every ~1 s. Endpoints (for scripting):
- `GET /status` → JSON with:
	- `temp` (C), `setpoint` (C)
	- `kp`, `ki`, `kd`, `manual` (bool), `power` (bool)
	- `out` (0..PWM_MAX), `pwmMax`
	- `spmode` (bool for POT mode)
	- `batV` (volts) and `batPct` (0..100), or `null` if battery monitor is disabled/not sampled yet
- `GET /control?...` → Accepts `sp`, `kp`, `ki`, `kd`, `mode=AUTO|MAN`, `pwr=0|1|toggle`, `out`

To disable Wi‑Fi, set `#define USE_WIFI 0` in the sketch.

## Optional: BLE control (CoreBluetooth on iOS)

The sketch includes an optional BLE GATT service (disabled by default to keep dependencies minimal). Enable by setting `#define USE_BLE 1`.

Service UUID: `b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a`

Characteristics (read/write unless noted):
- Enable (bool/"0" or "1"): `3f1a0001-2a8d-4a54-8f2f-b7cd2b4b8001`
- Setpoint (string/float): `3f1a0002-2a8d-4a54-8f2f-b7cd2b4b8001`
- Kp: `3f1a0003-2a8d-4a54-8f2f-b7cd2b4b8001`
- Ki: `3f1a0004-2a8d-4a54-8f2f-b7cd2b4b8001`
- Kd: `3f1a0005-2a8d-4a54-8f2f-b7cd2b4b8001`
- Mode ("AUTO"/"MAN"/"U1"/"U2"): `3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8001`
- Mode Read (notifications for mode sync): `3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8002` (Read/Notify: Int 0-4 for mode index)
- Output (int 0..1023): `3f1a0008-2a8d-4a54-8f2f-b7cd2b4b8001`
- Temperature (read/notify): `3f1a0007-2a8d-4a54-8f2f-b7cd2b4b8001`
- Battery % (read/notify): `3f1a0009-2a8d-4a54-8f2f-b7cd2b4b8001`
- Default Setpoint (write): `3f1a000a-2a8d-4a54-8f2f-b7cd2b4b8001` (Write: Float string, persisted in RTC)

You can test from iOS using the free "nRF Connect" app or the ESPAtomizer iOS app:
1) Scan and connect to "Atomizer".
2) Find the service by UUID; write values to change settings and subscribe to notifications.

Note: BLE uses NimBLE on ESP32. If your toolchain lacks `NimBLEDevice.h`, install "NimBLE-Arduino" and rebuild, or keep `USE_BLE` at 0.

## Power, Battery Logic, and Sleep Modes

- **Startup Behavior:**
  - When the device receives power (from battery or USB), it always powers on and boots the firmware.
  - The device restores its last saved state (setpoint, PID mode, output, etc.) from RTC memory.
  - If the last state was OFF, the device remains OFF; otherwise, it resumes operation automatically.
  - Physical hardware power switch (if present) fully disconnects battery and board; when switched ON, device boots immediately.

- **Battery Use and Unplugged Operation:**
  - The Atomizer always uses battery power if available. If unplugged from USB/external supply, it continues running on battery.
  - Device remains operational on battery unless explicitly turned off via button, BLE, or web interface.
  - All features (PID, BLE, OLED, encoder, etc.) are available on battery power.

- **Low Battery Handling:**
  - If battery voltage drops below the cutoff (`BAT_CUTOFF_V`), heating is disabled to protect the battery, but the device remains responsive for status, BLE, and UI.

- **Sleep Modes:**
  - The device enters deep sleep after a period of inactivity (default: 60 seconds, configurable via `SLEEP_ON_IDLE_MS`).
  - Deep sleep is aborted if any wake pins (encoder A/B/SW) are LOW, preventing unwanted sleep during user interaction.
  - On wake from sleep, the device restores its previous state from RTC memory and resumes operation.
  - The device will not enter deep sleep if the menu is active or recent activity is detected.

- **Power-Off Logic:**
  - The device only shuts down if the user turns it off (button press, BLE command, or web control).
  - Otherwise, it will run on battery until the battery is depleted or the device is manually powered off.

- **Physical Power Switch:**
  - If a hardware power switch is installed inline with the battery, switching it OFF fully disconnects power and turns off the device.
  - Switching ON restores power and the device boots immediately, regardless of previous software state.

---
