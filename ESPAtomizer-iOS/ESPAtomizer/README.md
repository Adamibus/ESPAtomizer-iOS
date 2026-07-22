# ESPAtomizer iOS ("Adamizer")

SwiftUI app for controlling an ESP32-based atomizer over Bluetooth LE: live
temperature monitoring, PID tuning, and per-mode temperature targets.

## Features

- **Live status card** — measured temperature (gray, dims when power is off),
  target beneath it, plus Output / Battery / Mode / Power stats.
- **Temperature chart** — fixed 15-second rolling window with axis ticks at
  0 / -5 / -10 / -15 s. The line breaks visibly across BLE dropouts instead of
  bridging missing time. Below the plot: an emerald output strip (duty cycle,
  fixed 0–100% scale) and the chart color picker.
- **Chart color picker** — five unlabeled swatches (emerald, amber, gold, plum,
  cream; gold is the default). The selection is persisted.
- **Modes** — Auto, Manual, U1, U2, and Config tabs. Auto/U1/U2 share one
  target + PID editor; each mode keeps its own target and tunings
  (all default 240 °C).
- **Safety-aware target range** — the target slider spans 165–315 °C, staying
  under the firmware's 320 °C thermal-runaway cutoff (`THERMAL_RUNAWAY_TEMP_C`).
- **Session history** — connected runs are logged to disk (`HistoryStore`) with
  periodic autosave.
- **Offline behavior** — controls that write to the device (target slider,
  manual output, PID save/reset) are disabled and dimmed while disconnected;
  the chart shows "Not connected" instead of an empty plot.

### The color rule

One rule everywhere: **the swatch colors what you set; gray is what the device
reports.**

- Swatch-colored: the target line in the chart, the target slider, the
  "Target …°C" label in the hero card, and the manual output slider.
- Gray (`Theme.measured`): the live temperature line and the big temperature
  readout.

User-facing text always says **Target**. "Setpoint" survives only in code and
in the BLE protocol names.

## Project layout

| File | Role |
| --- | --- |
| `ESPAtomizer_iOSApp.swift` | `@main` entry; locks dark mode, applies accent tint |
| `ContentView.swift` | All UI except the chart. Starts with `Theme` — the single palette/spacing/type-scale source |
| `TemperatureChartView.swift` | Chart (Swift Charts on iOS 16+, Canvas fallback), color picker, gap segmentation |
| `AtomizerViewModel.swift` | BLE (CoreBluetooth), state, persistence, string-based device protocol |
| `HistoryStore.swift` | Durable on-disk log of past sessions |

Useful constants in `AtomizerViewModel`: `chartWindowSeconds` (15 s window),
`historyGapThreshold` (5 s — a longer gap breaks the chart line), and
`configVersion` (bump it when a shipped default must override what existing
installs have saved; saved config otherwise wins forever).

## Building

Prerequisites: recent Xcode, and an Apple Developer account for device runs.

1. Open `ESPAtomizer-iOS.xcodeproj`.
2. Scheme/target: **ESPAtomizer**. Bundle id: `app.ESPAtomizer`.
3. Select your *Team* under *Signing & Capabilities* (automatic signing).
4. Run on a real iPhone to use Bluetooth — **the simulator has no BLE**, so it
   shows the disconnected UI. `Info.plist` already includes the Bluetooth
   usage strings.

Command line:

```sh
xcodebuild -project ESPAtomizer-iOS.xcodeproj -scheme ESPAtomizer \
  -destination 'platform=iOS Simulator,name=iPhone 17' build
```

## Troubleshooting

- **Signing errors** — pick your Team in *Signing & Capabilities*; keep
  *Automatically manage signing* checked for local development.
- **Bluetooth permission denied** — re-enable it in device
  *Settings → Privacy & Security → Bluetooth*.
- **"Cannot write: no peripheral"** — the app isn't connected; use the
  Connection card at the bottom of the page to scan and connect.
- **Stale build state** — *Product → Clean Build Folder*, and if needed
  `rm -rf ~/Library/Developer/Xcode/DerivedData/*`.
- **Changed a default but don't see it** — saved config overrides shipped
  defaults by design; bump `configVersion` in `AtomizerViewModel` to migrate
  existing installs.

For anything else, open an issue with the Xcode build log.
