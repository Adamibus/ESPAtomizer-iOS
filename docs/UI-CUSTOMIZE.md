# UI customization map

Every line in the iOS app worth editing, by category.
Paths are relative to `ESPAtomizer-iOS/ESPAtomizer/`.

---

## 1. Color — `ContentView.swift`

**Palette** (the raw colors) — lines **49–58**

    49  charcoal       #2A2C31
    50  charcoalLift   card background
    51  charcoalDeep   screen background
    52  emerald        #21B37D
    53  emeraldDim     54  emeraldGlow
    55  gold           #D4AF37
    56  goldDim
    57  cream          #F5F0E6
    58  creamMuted

**Roles** (what the colors mean) — lines **61–69**. Edit these to restyle
globally without touching the palette.

    61  accent    gold      titles, active controls
    62  bg        screen background
    63  card      panel background
    64  text      65  textDim
    66  ok        emerald — connected / power on
    67  warn      68  danger
    69  hairline  card border

Nothing else in the file hardcodes a color. Changing line 61 changes the
accent everywhere.

---

## 2. Shape and depth — `ContentView.swift`

    72  corner: CGFloat = 14      0 = sharp, 20+ = soft
    85–96  card()                the single panel recipe:
           87  .padding(Theme.pad)
           89  .background
           90  .cornerRadius
           92–93  border stroke + width
           95  .shadow(radius: 8, y: 3)

All 12 panels call `.card()`. Edit lines 85–96 once to restyle every panel.

---

## 3. Spacing — `ContentView.swift` lines 75–79

    75  sBlock  20   between major sections
    76  sCard   14   inside a card
    77  sRow     8   between related rows
    78  sTight   4   label above value
    79  pad     16   card inner padding

---

## 4. Typography — `ContentView.swift`

**Brand wordmark** — lines **138–139**

    138  .font(.system(size: 34, weight: .heavy, design: .rounded))
    139  .tracking(3)          letter spacing

**Hero temperature** — line **384** (`.largeTitle`). The highest-impact
single edit in the app. Try:

    .font(.system(size: 56, weight: .bold, design: .rounded))
    .monospacedDigit()        stops digits jittering as the value updates

**Secondary readouts** — `.title2` at 154, 158, 162, 396, 400, 410;
`.title3` at 421, 425, 435, 445

**Section headers** — `.headline` at 147, 236, 365, 541, 694, 793, 817,
876, 905, 922, 932, 960, 1027

**Field labels** — `.caption` at 371, 382, 392, 408, 418, 433, 443, 655,
1062, 1070, 1080, 1128; `.caption2` at 209, 1064, 1072, 1084, 1093, 1098

**Tab labels** — line 497 `.subheadline`

Add `.monospacedDigit()` to any live number: 384, 396, 400, 410, 420.

---

## 5. Controls

**Mode tab bar** — lines **495–505**. Fill/text colors at 500–501,
corner at 502, `.plain` style at 504.

**Buttons** — `.borderedProminent` at 593, 747, 869, 898, 915, 965, 1103,
1135; `.bordered` at 599, 604, 753, 758, 978, 1051, 1055, 1117, 1121, 1139

Global button tint: add `.tint(Theme.accent)` per button, or
`.accentColor(Theme.accent)` on the root view in `ESPAtomizer_iOSApp.swift`.

**Toggles** — 172 (power, tinted `Theme.ok`), 924, 1007
**Text fields** — `RoundedBorderTextFieldStyle()` at 546, 561, 576, 699,
714, 729, 824, 839, 854, 883, 907, 910
**Field widths** — 120pt at 548, 563, 578, 701, 716, 731; 80pt at 826, 841,
856; 100pt at 885

---

## 6. Motion — `ContentView.swift`

Only one animation exists today, line **205**:

    .animation(.default, value: selectedMode)

Better:

    .animation(.spring(response: 0.3, dampingFraction: 0.7), value: selectedMode)

Worth adding to the temperature value (384) and the connection dot (1022).

---

## 7. The chart — `TemperatureChartView.swift`

**Not yet themed. 12 hardcoded colors.**

    88   temperature line     Color.red      → Theme.accent
    95   setpoint line        Color.blue     → Theme.ok
    120  progress tint        Color.green    → Theme.ok
    130  area gradient        green          → Theme.accent
    140  panel background     systemGray6    → .card()
    211  canvas temp stroke   .red, width 2
    219  canvas setpoint      .blue, width 1.5
    230  progress tint        Color.green
    249  canvas gradient      green
    256  panel background     systemGray6

    113, 223  chart height 160
    136, 252  output bar height 44
    89, 96, 131  .interpolationMethod(.monotone)

---

## 8. Brand and copy

**Name** — `AtomizerViewModel.swift` lines **94–95** (`appTitle`, `appName`)

**Panel titles** — `ContentView.swift` 146 Controls · 364 Status ·
517 Auto Mode · 631 Manual Mode · 792 Device Info · 816 PID Tuning ·
875 Setpoint Configuration · 904 App Branding · 921 Notifications ·
931 Display Options · 959 Bluetooth Devices · 1026 Device Connection

**Status words** — 1031 Connected · 1035 Scanning... · 1039 Disconnected ·
444 ON/OFF · 208 POT/FIXED

**Warnings** — 654 manual-mode caution · 370 thermocouple fault ·
301 forget-device confirmation · 1128 bluetooth hint

---

## 9. Behavior limits

    ContentView.swift
    619, 773  setpoint range   30...315 °C  (86...599 °F)
    620, 774  slider step      0.5 °C / 1.0 °F
    952       history window   20...1000, step 10

    AtomizerViewModel.swift
    37, 103, 110  default setpoint 240 °C
    145           history limit 120
    223           min scan interval 2.0s

---

## 10. App-level

`ESPAtomizer_iOSApp.swift` — currently bare. The dark palette will fight
light mode until you add:

    ContentView()
        .preferredColorScheme(.dark)
        .tint(Theme.accent)

**Icon** — `Assets.xcassets/AppIcon.appiconset/`
**Toolbar icon** — `ContentView.swift` 229–232, `Image("app_icon")`, 30×30, corner 6

---

## Known issues

- **`appTitle`/`appName` never persist.** The "Save Branding" button
  (line 913) calls `saveConfigState()`, but neither field is written to or
  read from UserDefaults. Edits revert on relaunch.
- **RSSI is hardcoded to "—"** (line 1071) — never populated.
- **"Load from Device" buttons do nothing** (597–600, 751–754) — empty bodies.
