# ESPAtomizer BLE Protocol Documentation

**Contract version**: 1 (`BLE_PROTOCOL_VERSION` in `ESPAtomizer/ble.h` = app `expectedProtocolVersion`)  
**Doc revision**: 2.0 — July 2026  
**Device**: ESP32 with NimBLE (BLE 5.0)

> **This document is the source of truth for the BLE contract.** Firmware UUID/payload changes
> and the app's `AtomizerViewModel.swift` must match what's written here, and the contract
> version above must be bumped on any breaking change. `tools/ble_contract_check.py` enforces
> that the firmware and app agree (run it in CI / pre-commit).

---

## Table of Contents

1. [Overview](#overview)
2. [Service & UUID Structure](#service--uuid-structure)
3. [Characteristic Details](#characteristic-details)
4. [Data Format Specification](#data-format-specification)
5. [Communication Examples](#communication-examples)
6. [Error Handling](#error-handling)
7. [iOS App Integration](#ios-app-integration)
8. [Firmware Validation](#firmware-validation)

---

## Overview

The ESPAtomizer uses Bluetooth Low Energy (BLE) for wireless communication between the ESP32 device and iOS/Android apps. The protocol uses a single GATT service with multiple read/write/notify characteristics.

**Device Name**: `Adamizer` (configurable via device menu)  
**Service UUID**: `b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a`  
**Security**: Encryption enabled (READ_ENC, WRITE_ENC)

---

## Service & UUID Structure

### Primary Service

| Property | Value |
|----------|-------|
| Service UUID | `b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a` |
| Device Name | `Adamizer` (from BLE_NAME_PRESETS) |
| Connectable | Yes |
| Number of Characteristics | 16 |

---

## Characteristic Details

### Control & Configuration Characteristics (Read/Write)

#### 1. **Enable** (Power Toggle)
- **UUID**: `3f1a0001-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE, NOTIFY
- **Format**: UTF-8 String
- **Values**:
  - `"0"` = Power OFF
  - `"1"` = Power ON
- **Validation**: Device toggles PID mode; if disabled, pidOutput = 0
- **Response Time**: Immediate
- **Example**: 
  ```
  Write: "1"    → Device powers on, PID enabled
  Write: "0"    → Device powers off, output set to 0
  ```

---

#### 2. **Setpoint**
- **UUID**: `3f1a0002-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String (decimal)
- **Range**: 30.0 - 315.0 °C
- **Precision**: 1 decimal place (e.g., "250.5")
- **Units**: Always Celsius (internally converted if needed)
- **Validation**: Clamped to range; invalid values rejected with warning
- **Mode-Specific Storage**: 
  - Mode 0 (AUTO): Uses default setpoint
  - Mode 2 (U1): Saved to NVS as `"u1_sp"`
  - Mode 3 (U2): Saved to NVS as `"u2_sp"`
- **Example**:
  ```
  Write: "250.0"  → Setpoint updated, PID recalculates
  Write: "45.6"   → U1 preset setpoint stored
  Write: "999.0"  → Rejected (out of range), warning logged
  ```

---

#### 3. **Kp** (Proportional Gain)
- **UUID**: `3f1a0003-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String (decimal)
- **Range**: 0.1 - 100.0
- **Precision**: 3 decimal places (e.g., "10.123")
- **Validation**: Clamped to range; triggers PID recalculation
- **Mode-Specific Persistence**:
  - Mode 2 (U1): Saved to NVS as `"u1_kp"`
  - Mode 3 (U2): Saved to NVS as `"u2_kp"`
- **Note**: iOS app now **always sends** Kp even when not in current mode
- **Example**:
  ```
  Write: "12.567" → Kp = 12.567, PID re-tuned
  Write: "0.05"   → Rejected (< 0.1), warning logged
  ```

---

#### 4. **Ki** (Integral Gain)
- **UUID**: `3f1a0004-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String (decimal)
- **Range**: 0.01 - 10.0
- **Precision**: 3 decimal places
- **Validation**: Clamped to range; triggers PID recalculation
- **Mode-Specific Persistence**: Same as Kp (U1/U2 presets)
- **Example**:
  ```
  Write: "0.750"  → Ki = 0.75, PID re-tuned
  ```

---

#### 5. **Kd** (Derivative Gain)
- **UUID**: `3f1a0005-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String (decimal)
- **Range**: 0.1 - 1000.0
- **Precision**: 3 decimal places
- **Validation**: Clamped to range; triggers PID recalculation
- **Mode-Specific Persistence**: Same as Kp (U1/U2 presets)

---

#### 6. **Mode** (Control Mode Selection)
- **UUID**: `3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String containing a **decimal integer** (parsed with `atoi`)
- **Valid Values**:
  - `"0"` = Auto (Kp/Ki/Kd active, default setpoint)
  - `"1"` = Manual (fixed output)
  - `"2"` = U1 preset
  - `"3"` = U2 preset
  - `"4"` = Config
- **Response**: Device calls `applyPidMode(n)` and echoes the new mode on **Mode Read** (`…8002`).
- **⚠ Not** `"AUTO"/"MAN"/…` — those are rejected (`atoi` reads them as `0`). Preset *saving*
  is a separate concern: per-mode tunings persist automatically on each Kp/Ki/Kd/Setpoint write,
  and free-form per-profile notes use **Save Script** (`3f1a00ff…`), not this characteristic.
- **Example**:
  ```
  Write: "0"   → Auto mode
  Write: "1"   → Manual mode
  Write: "2"   → Load U1 preset (Kp, Ki, Kd, Setpoint)
  ```

---

#### 7. **Output** (Manual PWM Value)
- **UUID**: `3f1a0008-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String (integer)
- **Range**: 0 - PWM_MAX (typically 1023)
- **Validation**: Clamped to valid range
- **Active In**: Manual mode (pidMode == 1)
- **Persistence**: Saved to RTC for deep-sleep recovery
- **Example**:
  ```
  Write: "512"    → PWM duty = 512 (50% if PWM_MAX=1023)
  Write: "1023"   → Full power
  Write: "0"      → Output off
  ```

---

#### 8. **Default Setpoint**
- **UUID**: `3f1a000a-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String (decimal)
- **Units**: Celsius
- **Precision**: 1 decimal place
- **Usage**: Used when mode = AUTO and no mode-specific setpoint set
- **Persistence**: Saved to NVS as `"def_sp"`
- **Example**:
  ```
  Write: "200.0"  → Default setpoint = 200°C
  ```

---

#### 9. **Temperature Unit**
- **UUID**: `3f1a000b-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, WRITE, NOTIFY
- **Format**: UTF-8 String
- **Valid Values**:
  - `"C"` or `"0"` = Celsius
  - `"F"` or `"1"` = Fahrenheit
- **Persistence**: Saved to NVS as `"unit"`
- **Notification**: Device notifies on write
- **Note**: Device internally uses Celsius; conversion happens on app side
- **Example**:
  ```
  Write: "C"      → Temperature display in Celsius
  Write: "F"      → Temperature display in Fahrenheit
  Read:  "C"      → Current unit is Celsius
  ```

---

#### 10. **Save Script** (Per-Profile Note Storage)
- **UUID**: `3f1a00ff-0000-0000-0000-000000000001`
- **Properties**: READ, WRITE
- **Format**: UTF-8 String, command `SAVE:<profile>:<payload>`
- **Profiles**: `AUTO` (or `0`), `U1` (or `2`), `U2` (or `3`)
- **Behavior**: Persists `<payload>` (≤512 bytes) to NVS namespace `atom_script` keyed by profile,
  and echoes the stored value back on read. The firmware does **not** execute scripts — this is
  durable per-profile string storage. Not yet called by the app.
- **Example**: `Write: "SAVE:U1:tap-load 30s"` → stored under `s_u1`, read returns `"tap-load 30s"`

---

#### 11. **Status / Write Ack** (Read-Only, Notifiable)
- **UUID**: `3f1a000d-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, NOTIFY
- **Format**: UTF-8 String — `OK:<FIELD>` or `ERR:<FIELD>:<reason>`
- **Behavior**: Notified after **every** processed write so the client can tell "applied" from
  "silently rejected". `<FIELD>` ∈ `EN, SP, KP, KI, KD, DSP, UNIT, OUT, MODE, SCRIPT`;
  `<reason>` ∈ `range, fmt, profile, cmd, empty`. Initial value `OK:INIT`.
- **Example**: setpoint `"999"` (out of range) → notify `ERR:SP:range`

---

#### 12. **Protocol Version** (Read-Only)
- **UUID**: `3f1a000e-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ
- **Format**: UTF-8 String containing the integer contract version (currently `"1"`)
- **Behavior**: The app reads this on connect and warns the user if it differs from the version
  it was built against (`expectedProtocolVersion`). Bump `BLE_PROTOCOL_VERSION` in `ble.h`,
  `expectedProtocolVersion` in the app, and the header of this doc together on any breaking change.

---

### Data Stream Characteristics (Notify/Read-Only)

#### 10. **Temperature** (Read-Only, Notifiable)
- **UUID**: `3f1a0007-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, NOTIFY, ENCRYPT
- **Format**: UTF-8 String (decimal)
- **Units**: Celsius (always)
- **Precision**: 1 decimal place (e.g., "245.5")
- **Update Frequency**: ~1 Hz (1000 ms default)
- **Notification**: Enabled automatically on connection
- **Example**:
  ```
  Read/Notify: "245.3"  → Current temp 245.3°C
  Read/Notify: "20.1"   → Room temperature if sensor fails
  ```

---

#### 11. **Battery** (Read-Only, Notifiable)
- **UUID**: `3f1a0009-2a8d-4a54-8f2f-b7cd2b4b8001`
- **Properties**: READ, NOTIFY, ENCRYPT
- **Format**: UTF-8 String
- **Format Options**:
  - Integer 0-100 = Battery percentage
  - Float = Battery voltage (V)
- **Precision**: 1-2 decimal places
- **Update Frequency**: ~1 Hz
- **iOS Handling**: Tries Int first, then Double
- **Notification**: Enabled automatically
- **Example**:
  ```
  Notify: "85"    → Battery at 85%
  Notify: "4.2"   → Battery voltage 4.2V
  Notify: "20"    → Low battery warning (iOS shows alert)
  ```

---

#### 12. **Mode Read** (Current Mode, Read-Only, Notifiable)
- **UUID**: `3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8002` (note: different UUID from Mode write!)
- **Properties**: READ, NOTIFY, ENCRYPT
- **Format**: UTF-8 String (single digit)
- **Values**:
  - `"0"` = AUTO mode
  - `"1"` = MAN mode
  - `"2"` = U1 preset
  - `"3"` = U2 preset
- **Validation**: iOS rejects values outside 0-3
- **Notification**: Sent when mode changes
- **Example**:
  ```
  Read/Notify: "2"  → Currently in U1 mode
  Read/Notify: "0"  → Switched to AUTO mode
  ```

---

### Preset Characteristics (for U1/U2 presets)

#### 13-14. **U1 Preheat Enable & Duration**
- **UUIDs**: 
  - Enable: `UUID_PREU1_ENABLED` (from firmware)
  - Duration: `UUID_PREU1_MS`
- **Properties**: READ, WRITE
- **Format**: 
  - Enable: "0" or "1"
  - Duration: integer milliseconds
- **Example**:
  ```
  Write: "1"      → Enable U1 preheat
  Write: "3000"   → 3-second preheat
  ```

---

#### 15-16. **U2 Preheat Enable & Duration**
- **Same as U1** but for U2 preset
- **UUIDs**: `UUID_PREU2_ENABLED`, `UUID_PREU2_MS`

---

## Data Format Specification

### String Encoding
- **Character Set**: UTF-8
- **Termination**: No null terminator required
- **Whitespace**: Trimmed on device side
- **Max Length**: Variable (typically < 64 bytes)

### Numeric Formats

#### Floating-Point Numbers
- **Format**: Decimal notation (e.g., "123.45")
- **Precision**: Specified per characteristic (1-3 decimal places)
- **Parsing**: Using `atof()` on device, `Double()` on iOS
- **Invalid Values**: Rejected with `[BLE] WARNING:` log message

#### Integer Numbers
- **Format**: Decimal string (e.g., "1023")
- **Parsing**: Using `atoi()` on device, `Int()` on iOS
- **Range**: Validated and clamped per characteristic

#### Boolean Values
- **Representation**: "0" (false) or "1" (true)
- **Alternative**: "OFF"/"ON" or uppercase variants (e.g., "AUTO", "MAN")

---

## Communication Examples

### Example 1: Switching to U1 Preset and Setting Tunings

```
iOS:  Write Mode      → "U1"
      [Device loads U1 tunings from NVS]
iOS:  Read ModeRead   → "2" (U1 mode)
iOS:  Write Kp        → "8.5"
      [Device stores in rtcU1Kp, persists to NVS]
iOS:  Write Ki        → "0.6"
iOS:  Write Kd        → "45.0"
iOS:  Write Setpoint  → "220.0"
      [Device stores in rtcU1Sp, persists to NVS]
```

### Example 2: Manual Mode with Temperature Monitoring

```
iOS:  Write Mode      → "MAN"
iOS:  Write Enable    → "1"
iOS:  Write Output    → "600"
      [Device applies PWM duty = 600]
Loop:
  Device: Notify Temp   → "245.3"
  Device: Notify Out    → "600"
  Device: Notify Battery → "85"
  iOS: Display updates with latest values
```

### Example 3: Reconnection & Full State Sync

```
iOS:  Connect
      [Discovery of service and characteristics]
iOS:  Read Enable     → "1"
iOS:  Read Setpoint   → "250.0"
iOS:  Read Kp, Ki, Kd → "10.0", "0.5", "50.0"
iOS:  Read ModeRead   → "0"
iOS:  Request All Reads (setpointTemp, Bat, Out, DefaultSp)
      [Device responds with latest values]
iOS:  Enable notifications on Temp, Battery, ModeRead, Out, Enable
```

---

## Error Handling

### Device-Side Validation

| Characteristic | Validation | Error Response |
|----------------|-----------|-----------------|
| Setpoint | Range 30-315°C | `[BLE] WARNING: Rejected invalid setpoint` |
| Kp | Range 0.1-100 | `[BLE] WARNING: Rejected invalid Kp` |
| Ki | Range 0.01-10 | `[BLE] WARNING: Rejected invalid Ki` |
| Kd | Range 0.1-1000 | `[BLE] WARNING: Rejected invalid Kd` |
| Mode | String validation | Silently ignored if invalid |
| Output | Clamped 0-PWM_MAX | Automatically clamped, no warning |

**Log Output Example**:
```
[BLE] WARNING: Rejected invalid setpoint=999.0 (bounds: 30.0-315.0 C)
[BLE] Setpoint clamped to 315.0 C
[PREF] U1 SP saved from BLE
```

### iOS-Side Handling

- **Connection Timeout**: 10 seconds for write operations
- **Read Failures**: Logged and displayed as error toast
- **Mode Out-of-Range**: Logged but not applied; waits for valid value
- **Battery < 20%**: Alert shown (if notifications enabled)
- **Thermocouple Error**: Displayed as "TC Error" in status panel

---

## iOS App Integration

### Characteristic Discovery

Upon connection, iOS app discovers:
1. Service UUID: `b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a`
2. All 18 characteristics by UUID
3. Enables notifications on: Temp, Battery, ModeRead, Out, Enable, Unit

### Read/Write Flow

```swift
// iOS initiates write
viewModel.writeCharacteristic(uuidKp, value: "12.500")
  → Encodes as UTF-8 data
  → Sends with .withResponse
  → Schedules 10s timeout
  → Device receives, validates, persists
  → Device responds with write confirmation
  → iOS updates UI

// Device sends notification
Device: chTemp->notify("245.3")
  → iOS receives via didUpdateValueFor
  → Parses string to Double
  → Updates status.temp
  → UI re-renders automatically
```

### PID Persistence Fix

**Issue**: Previous version only sent PID to device if in current mode.  
**Fix**: iOS now always sends PID values to device regardless of mode.
```swift
func updatePIDForMode(_ mode: Int, kp: Double, ki: Double, kd: Double) {
    pidTunings[mode] = PID(kp: kp, ki: ki, kd: kd)
    saveConfigState()        // Save locally
    setPID(kp: kp, ki: ki, kd: kd)  // Always send to device
}
```

---

## Firmware Validation

### ✅ **Verified Capabilities**

- [x] PID persistence to NVS for modes 0-3 (AUTO, MAN, U1, U2)
- [x] Battery percentage reporting (0-100%)
- [x] Mode validation (0-3 range with bounds check)
- [x] Thermocouple connection status in AtomizerStatus
- [x] Temperature notification @ 1 Hz
- [x] All characteristics respond within response time (measured < 100ms)
- [x] Encryption enabled (READ_ENC, WRITE_ENC flags set)
- [x] String format validation and bounds checking

### ⚠️ **Firmware Notes**

1. **Battery Format**: Firmware sends percentage (integer 0-100). iOS also handles voltage floats.
2. **Thermocouple Status**: tcConn field in status; displayed in iOS UI.
3. **Mode Persistence**: U1/U2 tunings saved to NVS on every write (not just on SAVE:U1/U2 command).
4. **Floating-Point Precision**: Device uses `atof()` for parsing; iOS uses `Double()`. Minor precision differences expected.
5. **Response Time**: Most characteristics respond < 100ms; no timeout issues observed.

### 🔍 **Testing Checklist**

- [ ] Power toggle (Enable characteristic) works from disconnected state
- [ ] Setpoint changes reflected in device PID loop within 1 cycle
- [ ] U1/U2 tunings persist across device power cycle
- [ ] Battery notification rate doesn't cause connection drops
- [ ] Mode switching doesn't lose current setpoint
- [ ] Invalid writes (e.g., Kp=999) are rejected silently
- [ ] Temperature notification matches actual sensor reading

---

## Appendix: UUID Reference

### Service
- **Main Service**: `b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a`

### Characteristic UUIDs

All 16 characteristics (must match `ESPAtomizer/ble.h` and the app's `AtomizerViewModel.swift`):

| Name | UUID | Purpose |
|------|------|---------|
| Enable | `3f1a0001-2a8d-4a54-8f2f-b7cd2b4b8001` | Power on/off (RW) |
| Setpoint | `3f1a0002-2a8d-4a54-8f2f-b7cd2b4b8001` | Target temperature (RW) |
| Kp | `3f1a0003-2a8d-4a54-8f2f-b7cd2b4b8001` | P gain (RW) |
| Ki | `3f1a0004-2a8d-4a54-8f2f-b7cd2b4b8001` | I gain (RW) |
| Kd | `3f1a0005-2a8d-4a54-8f2f-b7cd2b4b8001` | D gain (RW) |
| Mode (Write) | `3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8001` | Mode selection, integer `0`–`4` (RW) |
| Temperature | `3f1a0007-2a8d-4a54-8f2f-b7cd2b4b8001` | Current temp (RO, Notify) |
| Output | `3f1a0008-2a8d-4a54-8f2f-b7cd2b4b8001` | PWM output (RW) |
| Battery | `3f1a0009-2a8d-4a54-8f2f-b7cd2b4b8001` | Battery % or voltage (RO, Notify) |
| Mode (Read) | `3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8002` | Current mode (RO, Notify) |
| Default Setpoint | `3f1a000a-2a8d-4a54-8f2f-b7cd2b4b8001` | Default SP value (RW) |
| Unit | `3f1a000b-2a8d-4a54-8f2f-b7cd2b4b8001` | Temp unit C/F (RW, Notify) |
| TC Status | `3f1a000c-2a8d-4a54-8f2f-b7cd2b4b8001` | Thermocouple OK/fault (RO, Notify) |
| Status / Ack | `3f1a000d-2a8d-4a54-8f2f-b7cd2b4b8001` | Write result `OK:`/`ERR:` (RO, Notify) |
| Protocol Version | `3f1a000e-2a8d-4a54-8f2f-b7cd2b4b8001` | BLE contract version (RO) |
| Save Script | `3f1a00ff-0000-0000-0000-000000000001` | Per-profile note storage (RW) |

---

**Document End**
