# ESPAtomizer iOS-Firmware Integration Testing Checklist

**Date Created**: December 30, 2025  
**Test Environment**: ESP32 XIAO C6 device + iOS 16.0+  
**Prerequisites**: BLE-PROTOCOL.md and FIRMWARE-BLE-AUDIT.md reviewed

---

## Pre-Test Setup

- [ ] Device firmware compiled and flashed (ESPAtomizer.ino)
- [ ] iOS app built and installed (ESPAtomizer-iOS)
- [ ] Both in same room with minimal BLE interference
- [ ] Serial monitor connected to device for log inspection
- [ ] Device battery > 50% for consistent testing
- [ ] Thermocouple sensor connected (for temperature tests)
- [ ] Heating element connected (for PID control tests)

---

## Module 1: BLE Connection & Discovery

### Test 1.1: Device Advertising
**Objective**: Verify device appears in iOS BLE scan  
**Steps**:
1. Open Settings > Bluetooth on iOS device
2. Power on ESP32 device
3. Wait 2 seconds for BLE advertising to start
4. Observe device list

**Expected Result**: "Adamizer" (or configured name) appears in Bluetooth list with strong signal  
**Pass Criteria**: Device appears within 3 seconds  
**Status**: [ ] PASS [ ] FAIL  
**Notes**: ___________________________________________

### Test 1.2: Initial Connection
**Objective**: Successfully establish BLE connection  
**Steps**:
1. Tap "Adamizer" in Settings > Bluetooth list
2. Observe pairing request
3. Accept pairing
4. Wait for connection confirmation

**Expected Result**: Device shows as "Connected" in Bluetooth settings  
**Pass Criteria**: Connection established within 5 seconds  
**Status**: [ ] PASS [ ] FAIL  
**Notes**: ___________________________________________

### Test 1.3: Service & Characteristic Discovery
**Objective**: iOS app discovers all BLE characteristics  
**Steps**:
1. Launch ESPAtomizer iOS app
2. Tap "Connect" or wait for auto-discovery
3. Observe loading progress

**Expected Result**: App shows device name, firmware version, all tabs functional  
**Pass Criteria**: No error messages; all UI elements load  
**Status**: [ ] PASS [ ] FAIL  
**Notes**: ___________________________________________

### Test 1.4: Notification Enablement
**Objective**: iOS successfully subscribes to device notifications  
**Steps**:
1. Open Serial monitor on device
2. Observe firmware logs during app connection
3. Check for subscription confirmations

**Expected Result**: Firmware logs show "Client subscribed to notifications"  
**Pass Criteria**: Logs confirm subscription for Temp, Battery, Mode, Output  
**Status**: [ ] PASS [ ] FAIL  
**Firmware Log**: ___________________________________________

---

## Module 2: Power Control & Enable/Disable

### Test 2.1: Power Toggle On
**Objective**: Power on device from iOS  
**Steps**:
1. Ensure device is OFF (status shows "Power: OFF")
2. Tap power toggle button (top-right)
3. Wait 1 second
4. Observe status panel

**Expected Result**: Status changes to "Power: ON"; heating element energizes  
**Pass Criteria**: Power state changes immediately  
**Status**: [ ] PASS [ ] FAIL  
**Device Behavior**: ___________________________________________

### Test 2.2: Power Toggle Off
**Objective**: Power off device from iOS  
**Steps**:
1. Ensure device is ON
2. Tap power toggle button
3. Observe status panel
4. Check device LED/heating element

**Expected Result**: Status changes to "Power: OFF"; heating stops  
**Pass Criteria**: Device powers off within 1 second  
**Status**: [ ] PASS [ ] FAIL  
**Device Behavior**: ___________________________________________

### Test 2.3: Device Enable via BLE Write
**Objective**: Verify BLE write for Enable characteristic  
**Steps**:
1. Open Xcode/console for iOS app debugging
2. Enable power
3. Observe BLE data sent (check with CoreBluetooth logs)

**Expected Result**: Write to UUID `3f1a0001-...` with value "1"  
**Pass Criteria**: No write errors logged  
**Status**: [ ] PASS [ ] FAIL  
**BLE Logs**: ___________________________________________

---

## Module 3: Temperature Reading & Display

### Test 3.1: Temperature Notification Reception
**Objective**: iOS receives temperature updates from device  
**Steps**:
1. Power on device with thermocouple connected
2. Open Serial monitor and note temperature value
3. Observe temperature display in iOS app Status panel
4. Wait 3 seconds and compare values

**Expected Result**: iOS displays same temperature as device console  
**Pass Criteria**: Values match within 1°C  
**Device Temp**: ___________ iOS Temp: ___________  
**Status**: [ ] PASS [ ] FAIL

### Test 3.2: Temperature Update Frequency
**Objective**: Verify temperature updates at ~1 Hz  
**Steps**:
1. Enable temperature notifications
2. Record temperature readings in iOS app for 10 seconds
3. Count number of unique temperature updates

**Expected Result**: 8-12 unique temperature values in 10 seconds (~1 Hz)  
**Pass Criteria**: Update frequency is 0.8-1.2 Hz  
**Update Count**: ___________ in 10 seconds  
**Status**: [ ] PASS [ ] FAIL

### Test 3.3: Temperature Precision
**Objective**: Verify temperature displayed with correct decimal places  
**Steps**:
1. Observe temperature in Status panel
2. Note decimal place precision
3. Compare with firmware output

**Expected Result**: 1 decimal place (e.g., "245.3°C")  
**Pass Criteria**: Consistent 1 decimal place display  
**Sample Values**: 
- Firmware: ___________
- iOS: ___________  
**Status**: [ ] PASS [ ] FAIL

---

## Module 4: PID Tuning & Persistence

### Test 4.1: Setpoint Change in AUTO Mode
**Objective**: Change setpoint and verify device receives it  
**Steps**:
1. Switch to AUTO mode tab
2. Drag setpoint slider to 200°C
3. Release slider
4. Observe device behavior

**Expected Result**: Device PID setpoint updates to 200°C  
**Pass Criteria**: Temperature begins rising toward 200°C  
**Device Output**: ___________ W  
**Status**: [ ] PASS [ ] FAIL

### Test 4.2: Kp Gain Modification
**Objective**: Modify proportional gain and verify persistence  
**Steps**:
1. Stay in AUTO mode
2. Drag Kp slider to 15.0
3. Release slider
4. Observe how device responds to temperature changes

**Expected Result**: Device updates Kp and adjusts PID response  
**Pass Criteria**: Device responsiveness changes noticeably  
**Before Kp**: __________ After Kp: __________  
**Status**: [ ] PASS [ ] FAIL

### Test 4.3: U1 Preset Programming
**Objective**: Set and save U1 preset from iOS  
**Steps**:
1. Switch to U1 tab
2. Set Kp=8.5, Ki=0.6, Kd=45.0, Setpoint=220°C
3. Observe device updates
4. Verify preset is set correctly

**Expected Result**: Device loads U1 values and maintains them  
**Pass Criteria**: All 4 values match within 0.1 (PID) or 1 (Setpoint)  
**Set Values**: Kp=8.5, Ki=0.6, Kd=45.0, Sp=220°C  
**Status**: [ ] PASS [ ] FAIL

### Test 4.4: U1 Preset Persistence Across Power Cycle
**Objective**: Verify U1 preset survives device power-off/on  
**Steps**:
1. Set U1 preset (from Test 4.3)
2. Power off device completely (via button or disconnect battery)
3. Wait 10 seconds
4. Power on device
5. Select U1 mode from iOS
6. Read Kp, Ki, Kd, Setpoint values via iOS app

**Expected Result**: All 4 values match previously set values  
**Pass Criteria**: Kp=8.5±0.01, Ki=0.6±0.01, Kd=45.0±0.1, Sp=220±1  
**Read Values**: Kp=___, Ki=___, Kd=___, Sp=___  
**Status**: [ ] PASS [ ] FAIL

### Test 4.5: U2 Preset Programming & Persistence
**Objective**: Repeat Tests 4.3-4.4 for U2 preset  
**Steps**:
1. Switch to U2 tab
2. Set Kp=7.0, Ki=0.4, Kd=50.0, Setpoint=180°C
3. Power cycle device
4. Select U2 mode
5. Verify values persist

**Expected Result**: All U2 values persist correctly  
**Pass Criteria**: All values match within 0.1 (PID) or 1 (Setpoint)  
**Set Values**: Kp=7.0, Ki=0.4, Kd=50.0, Sp=180°C  
**Read Values**: Kp=___, Ki=___, Kd=___, Sp=___  
**Status**: [ ] PASS [ ] FAIL

### Test 4.6: Auto Mode Default Setpoint
**Objective**: Verify AUTO mode uses default setpoint  
**Steps**:
1. Switch to Config tab
2. Set Default Setpoint to 200°C
3. Save config
4. Switch to AUTO mode
5. Observe temperature ramps toward 200°C

**Expected Result**: AUTO mode uses 200°C setpoint  
**Pass Criteria**: Temperature rises toward 200°C  
**Status**: [ ] PASS [ ] FAIL

### Test 4.7: Mode Switching Setpoint Isolation
**Objective**: Verify each mode has independent setpoint  
**Steps**:
1. In AUTO mode, set setpoint to 250°C
2. Switch to U1 mode (setpoint: 220°C)
3. Verify temperature ramps toward 220°C
4. Switch back to AUTO
5. Verify temperature ramps toward 250°C

**Expected Result**: Each mode maintains its own setpoint  
**Pass Criteria**: Temperature setpoint changes when mode switches  
**AUTO Sp**: 250°C, U1 Sp: 220°C  
**Temperature behavior**: ___________________________________________  
**Status**: [ ] PASS [ ] FAIL

---

## Module 5: Manual Mode & Output Control

### Test 5.1: Manual Mode PWM Output
**Objective**: Set fixed PWM output in manual mode  
**Steps**:
1. Switch to Manual mode tab
2. Drag PWM slider to 50% (512 out of 1023)
3. Release slider
4. Observe heating element power

**Expected Result**: Device applies 50% PWM to heating element  
**Pass Criteria**: Heating element power approximately constant  
**PWM Value**: 512 (50%)  
**Status**: [ ] PASS [ ] FAIL

### Test 5.2: PWM Output Range
**Objective**: Verify PWM output respects 0-1023 range  
**Steps**:
1. Attempt to set PWM to 0 (min)
2. Verify heating stops
3. Set PWM to 1023 (max)
4. Verify maximum heating power

**Expected Result**: PWM constrains to 0-1023 range  
**Pass Criteria**: No values outside range allowed  
**Min Output**: 0, Max Output: 1023  
**Status**: [ ] PASS [ ] FAIL

### Test 5.3: PWM Output Persistence
**Objective**: Verify PWM value survives mode switch  
**Steps**:
1. Set Manual mode PWM to 600
2. Switch to AUTO mode
3. Switch back to Manual mode
4. Verify PWM still shows 600

**Expected Result**: PWM value persists in memory  
**Pass Criteria**: PWM reads back as 600±10  
**Status**: [ ] PASS [ ] FAIL

---

## Module 6: Battery Monitoring

### Test 6.1: Battery Percentage Display
**Objective**: Verify battery percentage shown in status  
**Steps**:
1. Ensure device is fully charged (battery > 95%)
2. Open Status panel
3. Observe battery percentage display

**Expected Result**: Battery shows 90-100%  
**Pass Criteria**: Value is integer 0-100  
**Battery Reading**: __________ %  
**Status**: [ ] PASS [ ] FAIL

### Test 6.2: Battery Notification Frequency
**Objective**: Verify battery updates at ~1 Hz  
**Steps**:
1. Enable battery notifications
2. Record battery readings for 10 seconds
3. Count number of unique values

**Expected Result**: 8-12 unique updates in 10 seconds  
**Pass Criteria**: Frequency is 0.8-1.2 Hz  
**Update Count**: ___________ in 10 seconds  
**Status**: [ ] PASS [ ] FAIL

### Test 6.3: Battery Low Alert
**Objective**: Verify alert triggers when battery < 20%  
**Steps**:
1. Simulate low battery (if device supports testing mode)
   - OR manually set battery level < 20% in firmware
2. Observe iOS app
3. Check for alert dialog

**Expected Result**: Alert appears saying "Battery Low"  
**Pass Criteria**: Alert appears when battery < 20% and notificationsEnabled=true  
**Status**: [ ] PASS [ ] FAIL  
**Notes**: ___________________________________________

---

## Module 7: BLE Communication & Slider Debouncing

### Test 7.1: Setpoint Slider Debouncing
**Objective**: Verify slider writes are debounced to single write on release  
**Steps**:
1. Enable firmware serial logging or BLE packet capture
2. Switch to AUTO mode
3. Drag setpoint slider continuously for 2 seconds (multiple positions)
4. Count number of BLE writes to UUID_SETPOINT

**Expected Result**: Single write on release, not multiple writes during drag  
**Pass Criteria**: Only 1 write to setpoint characteristic per slider release  
**Writes Observed**: __________ (should be 1 per release, not per frame)  
**Status**: [ ] PASS [ ] FAIL

### Test 7.2: Manual PWM Slider Debouncing
**Objective**: Verify PWM slider is debounced  
**Steps**:
1. Switch to Manual mode
2. Drag PWM slider continuously for 2 seconds
3. Count BLE writes to UUID_OUTPUT

**Expected Result**: Single write on release  
**Pass Criteria**: 1 write per slider release  
**Writes Observed**: __________  
**Status**: [ ] PASS [ ] FAIL

### Test 7.3: Characteristic Write Timeout
**Objective**: Verify writes complete within 10-second timeout  
**Steps**:
1. Perform 10 setpoint changes rapidly
2. Observe iOS for timeout errors
3. Check Serial monitor for response times

**Expected Result**: All writes complete without timeout  
**Pass Criteria**: No timeout errors; all responses < 10 seconds  
**Response Time**: Average __________ ms  
**Status**: [ ] PASS [ ] FAIL

---

## Module 8: Temperature Unit Display

### Test 8.1: Temperature Unit Selection
**Objective**: Switch between Celsius and Fahrenheit  
**Steps**:
1. Switch to Config tab
2. Select unit as °C
3. Observe temperature in Status panel
4. Switch unit to °F
5. Observe temperature change

**Expected Result**: Temperature converted correctly (°C × 9/5 + 32 = °F)  
**Pass Criteria**: Conversion is accurate within 1°F  
**Temp in °C**: __________, Temp in °F: __________  
**Status**: [ ] PASS [ ] FAIL

### Test 8.2: Unit Persistence
**Objective**: Verify unit setting persists across app restart  
**Steps**:
1. Set unit to °F
2. Close app completely
3. Reopen app
4. Check unit setting

**Expected Result**: Unit is still °F  
**Pass Criteria**: User preference persists  
**Status**: [ ] PASS [ ] FAIL

---

## Module 9: Reconnection & Error Handling

### Test 9.1: Accidental Disconnection Recovery
**Objective**: iOS automatically reconnects if device loses connection  
**Steps**:
1. Connect to device
2. Move away from device (out of BLE range)
3. Wait 10 seconds
4. Move back into range
5. Observe iOS app status

**Expected Result**: App shows "Connecting..." then reconnects automatically  
**Pass Criteria**: Auto-reconnect succeeds within 30 seconds  
**Reconnect Time**: __________ seconds  
**Status**: [ ] PASS [ ] FAIL

### Test 9.2: Manual Reconnection
**Objective**: User can manually reconnect after disconnect  
**Steps**:
1. In iOS app, tap "Forget Device" or disconnect
2. Tap "Connect" to reconnect
3. Observe connection status

**Expected Result**: App reconnects successfully  
**Pass Criteria**: Connection re-established within 5 seconds  
**Status**: [ ] PASS [ ] FAIL

### Test 9.3: Invalid Write Error Handling
**Objective**: Device rejects invalid values gracefully  
**Steps**:
1. Attempt to set Kp to 500 (out of range 0.1-100)
2. Observe iOS for error message
3. Check device logs

**Expected Result**: Device silently clamps value or rejects write; iOS shows error  
**Pass Criteria**: No crash; error logged  
**Firmware Log**: ___________________________________________  
**Status**: [ ] PASS [ ] FAIL

---

## Module 10: Thermocouple Status (⚠️ PENDING FIRMWARE IMPLEMENTATION)

### Test 10.1: Thermocouple Connected Status
**Objective**: Display "TC: OK" when thermocouple is connected  
**Steps**:
1. Ensure thermocouple sensor is connected
2. Open Status panel
3. Look for "TC: OK" text (green)

**Expected Result**: Status shows "TC: OK" in green  
**Pass Criteria**: Correct status and color shown  
**Status**: [ ] PASS [ ] FAIL [ ] SKIP (firmware not yet updated)

### Test 10.2: Thermocouple Disconnected Status
**Objective**: Display "TC: Error" when thermocouple is disconnected  
**Steps**:
1. Disconnect thermocouple sensor
2. Wait 2 seconds
3. Observe Status panel

**Expected Result**: Status shows "TC: Error" in red  
**Pass Criteria**: Status updates within 2 seconds of disconnect  
**Status**: [ ] PASS [ ] FAIL [ ] SKIP (firmware not yet updated)

---

## Module 11: Stress Testing

### Test 11.1: Rapid Mode Switching
**Objective**: Verify device handles rapid mode changes  
**Steps**:
1. Switch mode every 1 second (AUTO → U1 → AUTO → U2 → ...)
2. Continue for 30 seconds
3. Observe for crashes or disconnects

**Expected Result**: No crashes; PID continues to work  
**Pass Criteria**: Mode switches complete without errors  
**Status**: [ ] PASS [ ] FAIL

### Test 11.2: Rapid Setpoint Changes
**Objective**: Verify device handles rapid temperature setpoint updates  
**Steps**:
1. Change setpoint every 0.5 seconds (200 → 210 → 220 → ... → 290)
2. Observe temperature tracking
3. Check for connection drops

**Expected Result**: All changes applied; temperature follows  
**Pass Criteria**: No disconnections or errors  
**Status**: [ ] PASS [ ] FAIL

### Test 11.3: Long-Duration Operation
**Objective**: Verify device remains stable for extended operation  
**Steps**:
1. Set device to AUTO mode with setpoint 250°C
2. Let run for 1 hour
3. Monitor temperature stability
4. Check for disconnections

**Expected Result**: Device maintains temperature within ±5°C; no disconnects  
**Pass Criteria**: Stable operation for full hour  
**Final Temp**: __________ °C  
**Disconnections**: __________ (should be 0)  
**Status**: [ ] PASS [ ] FAIL

---

## Module 12: iOS App Specific Tests

### Test 12.1: Multiple Connections (if supported)
**Objective**: Verify app handles multiple device connections  
**Steps**:
1. If firmware supports multiple connections, pair second device
2. Switch between devices in iOS app
3. Observe functionality

**Expected Result**: App correctly switches focus to selected device  
**Pass Criteria**: No data cross-talk between devices  
**Status**: [ ] PASS [ ] FAIL [ ] NOT SUPPORTED

### Test 12.2: Background App Refresh
**Objective**: Verify app behavior when backgrounded  
**Steps**:
1. Connect to device
2. Switch to Home screen (background app)
3. Wait 5 seconds
4. Return to app

**Expected Result**: App resumes without reconnection required  
**Pass Criteria**: App resyncs data within 2 seconds  
**Status**: [ ] PASS [ ] FAIL

### Test 12.3: App Crash Recovery
**Objective**: Verify app state persists after crash  
**Steps**:
1. Set custom PID tunings
2. Force-close app
3. Reopen app
4. Check if settings persist

**Expected Result**: Settings saved to UserDefaults; app recovers state  
**Pass Criteria**: All custom settings intact  
**Status**: [ ] PASS [ ] FAIL

---

## Summary & Sign-Off

### Test Results Overview

| Module | Tests Passed | Tests Failed | Tests Skipped | Status |
|--------|-------------|-------------|---------------|--------|
| 1. BLE Connection | ___ / 4 | ___ | ___ | [ ] |
| 2. Power Control | ___ / 3 | ___ | ___ | [ ] |
| 3. Temperature | ___ / 3 | ___ | ___ | [ ] |
| 4. PID & Persistence | ___ / 7 | ___ | ___ | [ ] |
| 5. Manual Mode | ___ / 3 | ___ | ___ | [ ] |
| 6. Battery | ___ / 3 | ___ | ___ | [ ] |
| 7. BLE Comm | ___ / 3 | ___ | ___ | [ ] |
| 8. Temperature Unit | ___ / 2 | ___ | ___ | [ ] |
| 9. Reconnection | ___ / 3 | ___ | ___ | [ ] |
| 10. Thermocouple | ___ / 2 | ___ | ⚠️ 2 | [ ] |
| 11. Stress Tests | ___ / 3 | ___ | ___ | [ ] |
| 12. iOS App | ___ / 3 | ___ | ___ | [ ] |
| **TOTAL** | **___ / 46** | **___** | **⚠️ 2** | **[ ]** |

### Pass/Fail Criteria

- **PASS**: All tests in Modules 1-9, 11-12 must pass
- **ACCEPTABLE**: Module 10 (Thermocouple) can SKIP if firmware not yet updated
- **RELEASE BLOCKER**: Any FAIL in Modules 1-9, 11-12

### Sign-Off

**Tested By**: ___________________________ **Date**: _______________

**Result**: 
- [ ] **APPROVED** - All critical tests passed; app ready for release
- [ ] **APPROVED WITH NOTES** - Tests passed; known issues documented below
- [ ] **REJECTED** - Critical failures; retry after fixes

**Known Issues / Notes**:
```
1. ________________________________________________________________________
2. ________________________________________________________________________
3. ________________________________________________________________________
```

**Recommendations for Next Release**:
```
1. Implement thermocouple BLE characteristic (currently displays stale value)
2. ________________________________________________________________________
3. ________________________________________________________________________
```

---

**Test Report Complete**

*For issues found, file GitHub issues with "test-failure:" prefix and include test module number and name.*

