# OLED Hardware Testing Checklist

## Pre-Testing Setup

### Hardware Requirements
- [ ] ESP32-C6 microcontroller
- [ ] SSD1306 128x64 OLED display
- [ ] Thermocouple with MAX31855/MAX6675 amplifier
- [ ] Rotary encoder with button
- [ ] Battery with voltage monitoring
- [ ] USB cable for serial monitoring
- [ ] Computer with Arduino IDE or serial terminal

### Software Requirements
- [ ] Latest firmware flashed (ESPAtomizer.ino)
- [ ] Serial monitor configured (115200 baud)
- [ ] BLE device ready for connection testing (smartphone)

### Initial Checks
- [ ] OLED displays "Booting..." message on power-up
- [ ] Serial monitor shows OLED initialization messages
- [ ] No I2C errors in serial log
- [ ] Display shows normal operating screen after boot

---

## Test 1: Basic Display Functionality

### 1.1 Power-On Sequence
- [ ] Connect power to device
- [ ] Verify "Booting..." message appears on OLED
- [ ] Wait for boot sequence to complete
- [ ] Verify normal operating screen appears
- [ ] Check serial log for OLED init success message

**Expected Serial Output:**
```
[OLED] Init successful at address 0x3C
```

### 1.2 Display Elements
- [ ] Top line shows `PWR:OFF [AUTO] BAT:XX%`
- [ ] Status line shows BLE or fault message
- [ ] Temperature reading shown (or `---` if sensor disconnected)
- [ ] Setpoint shown (format: `SP:XXX.XC`)
- [ ] Output bar visible at bottom
- [ ] Output percentage shown (right-aligned)

### 1.3 Update Rate
- [ ] Watch temperature reading - should update smoothly
- [ ] No visible flickering or artifacts
- [ ] Display updates ~5-10 times per second
- [ ] Check serial log - update time should be <100ms

**Expected Serial Output:**
```
[OLED] Update time: 35ms
```

---

## Test 2: Management Feature Integration

### 2.1 Error Display
**Setup:** Disconnect thermocouple to trigger sensor fault

- [ ] Wait 2-3 seconds for fault detection
- [ ] Verify `E:1` appears on status line
- [ ] Verify "SENSOR FAULT" message displayed
- [ ] Verify temperature shows `---`
- [ ] Check serial log for error logged

**Expected Serial Output:**
```
[ERROR] Sensor: Thermocouple read failed/disconnected (count=1)
```

**Cleanup:**
- [ ] Reconnect thermocouple
- [ ] Wait for fault to clear
- [ ] Send `MR` command via serial to reset error count
- [ ] Verify `E:1` disappears from display

### 2.2 System Halt Display
**Setup:** Trigger 10+ sensor faults or send `MH` command to check health

- [ ] Method 1: Disconnect/reconnect sensor 10 times rapidly
- [ ] Method 2: Use serial command to force error threshold
- [ ] Verify `!ERR` appears on status line (replacing `E:X`)
- [ ] Verify system disabled (`PWR:OFF`)
- [ ] Verify health message displayed on status line
- [ ] Check serial log for system halt message

**Expected OLED Display:**
```
PWR:OFF [AUTO] !ERR  BAT:XX%
Too many errors: halted
---
SP:250.0C
Output: 0%
```

**Expected Serial Output:**
```
[HEALTH] System halted due to excessive errors
```

**Cleanup:**
- [ ] Send `MR` command to reset errors
- [ ] Send `MH` command to verify health restored
- [ ] Verify `!ERR` disappears

### 2.3 Runtime Warning
**Setup:** Run system continuously for 25+ minutes

⚠️ **Note:** This test takes 25+ minutes. Consider temporarily reducing threshold for testing:
```cpp
// Temporary modification for testing (line ~2738)
if (runtimeMs > 1500000UL) { // 25 minutes
// Change to:
if (runtimeMs > 60000UL) { // 1 minute (for testing)
```

- [ ] Enable system (press encoder button)
- [ ] Set output >0% (verify heating)
- [ ] Wait for threshold time
- [ ] Verify `!` icon appears next to output percentage
- [ ] Continue running to 30 minutes
- [ ] Verify cooldown mode activates automatically

**Expected OLED Display (at 25+ minutes):**
```
PWR:ON [AUTO]        BAT:XX%
CON
245.3C
SP:250.0C
Output: 63% !  ← Warning icon
```

**Expected Serial Output:**
```
[SAFETY] 30-minute runtime limit reached - entering cooldown
```

### 2.4 Cooldown Timer
**Setup:** Continue from previous test or manually trigger cooldown

- [ ] Wait for 30-minute limit (or reduce for testing)
- [ ] Verify output drops to 0%
- [ ] Verify "COOLDOWN: XXs" message appears (highest priority)
- [ ] Watch countdown timer decrease
- [ ] Verify timer counts down from 60 seconds
- [ ] Wait for cooldown to complete
- [ ] Verify normal operation resumes

**Expected OLED Display:**
```
PWR:ON [AUTO]        BAT:XX%
COOLDOWN: 45s  ← Countdown
87.2C
SP:250.0C
Output: 0%
```

**Cleanup:**
- [ ] Disable system (press encoder)
- [ ] Re-enable to verify normal operation

---

## Test 3: Fault Indicators

### 3.1 Sensor Fault
- [ ] Disconnect thermocouple
- [ ] Verify "SENSOR FAULT" message appears
- [ ] Verify temperature shows `---`
- [ ] Verify warning stripes appear on output bar
- [ ] Verify output disabled (0%)
- [ ] Reconnect sensor
- [ ] Verify fault clears within 2-3 seconds

### 3.2 Thermal Runaway
**Setup:** Manually trigger thermal runaway (requires special conditions)

⚠️ **Note:** Difficult to test safely. Verify code logic instead:
```cpp
// Check these variables exist and are used:
thermalRunawayFaulted = true;  // Set when detected
```

**Expected OLED Display:**
```
PWR:ON [AUTO] E:X    BAT:XX%
THERMAL FAULT
XXX.XC
SP:250.0C
Output: /\/\/\  ← Warning stripes
```

### 3.3 Battery Low
**Setup:** Discharge battery below threshold or modify threshold temporarily

**Method 1 - Actual Discharge:**
- [ ] Operate device until battery low
- [ ] Verify "BATTERY LOW" message appears
- [ ] Verify filled circle icon `●` shown next to output %

**Method 2 - Temporary Threshold:**
```cpp
// Modify BAT_MIN_V temporarily for testing (line ~XXX in config.h)
#define BAT_MIN_V 3.0  // Normal value
// Change to:
#define BAT_MIN_V 10.0  // Force battery low for testing
```

- [ ] Flash modified firmware
- [ ] Power on device
- [ ] Verify "BATTERY LOW" message
- [ ] Verify battery icon displayed
- [ ] Restore original threshold

**Expected OLED Display:**
```
PWR:ON [AUTO] E:X    BAT:12%
BATTERY LOW
245.3C
SP:250.0C
Output: ● 50%  ← Battery icon
```

### 3.4 Warning Stripes
- [ ] Trigger any fault (sensor, thermal, or watchdog)
- [ ] Observe output bar at bottom
- [ ] Verify diagonal stripe pattern visible
- [ ] Pattern should be: `/\/\/\/\/\`
- [ ] Stripes overlay normal bar fill
- [ ] Clear fault
- [ ] Verify stripes disappear

---

## Test 4: Display Modes

### 4.1 Menu Overlay
- [ ] Press and hold encoder button (short press)
- [ ] Verify menu overlay appears centered
- [ ] Menu should show 8 PID modes
- [ ] Selected item highlighted (inverted colors)
- [ ] Rotate encoder to change selection
- [ ] Verify highlight moves up/down
- [ ] Press button to select mode
- [ ] Verify menu closes
- [ ] Check serial log for mode change

**Expected OLED Display:**
```
┌────────────────┐
│►OFF           │
│ Hold 100%      │
│ Hold  75%      │
│ Hold  50%      │
│ Hold  25%      │
│ Auto Normal    │
│ Auto Aggres... │
│ Auto Conserv...│
└────────────────┘
```

### 4.2 Config Mode
- [ ] Press and hold encoder button for 2+ seconds
- [ ] Verify config screen appears
- [ ] Verify border rectangle visible
- [ ] Config items should be listed vertically
- [ ] Selected item highlighted (inverted colors)
- [ ] Values shown on right side
- [ ] Rotate encoder to scroll through items
- [ ] Verify scroll arrows appear (▲▼) when items off-screen

**Expected OLED Display:**
```
┌─────────────────────┐
│ Default SP   250.0C │
│►Unit             C  │
│ BLE ON          ON  │
│ Name          Dev1  │
│ AdvInt        200ms │
│ OLED           ON   │
│ Save           OK   │
│ Factory Reset  RST  │
└─────────────────────┘
```

### 4.3 Config Item: Default SP (Item 0)
- [ ] Scroll to "Default SP"
- [ ] Current value shown on right (e.g., `250.0C`)
- [ ] Press encoder button to cycle value
- [ ] Value should change (200°C → 225°C → 250°C → 275°C → 300°C → cycle)
- [ ] Check serial log for confirmation

**Expected Serial Output:**
```
[PREF] Default setpoint saved: 275.0
```

### 4.4 Config Item: Unit (Item 1)
- [ ] Scroll to "Unit"
- [ ] Current value shown: `C` or `F`
- [ ] Press encoder button to toggle
- [ ] Value should alternate: `C` ↔ `F`
- [ ] Verify temperature display updates immediately
- [ ] Check serial log for confirmation

### 4.5 Config Item: BLE ON (Item 2)
- [ ] Scroll to "BLE ON"
- [ ] Current value: `ON` or `OFF`
- [ ] Press encoder button to toggle
- [ ] Check serial log for BLE start/stop messages

### 4.6 Config Item: Name (Item 3)
- [ ] Scroll to "Name"
- [ ] Current preset shown (e.g., `Dev1`)
- [ ] Press encoder button to cycle through 4 presets
- [ ] Presets: Dev1 → Dev2 → Dev3 → Dev4 → cycle
- [ ] Check serial log for name change

### 4.7 Config Item: AdvInt (Item 4)
- [ ] Scroll to "AdvInt"
- [ ] Current interval shown (100ms/200ms/1000ms)
- [ ] Press encoder button to cycle
- [ ] Values: 100ms → 200ms → 1000ms → cycle
- [ ] Check serial log for confirmation

### 4.8 Config Item: OLED (Item 5)
- [ ] Scroll to "OLED"
- [ ] Current state: `ON` or `OFF`
- [ ] Press encoder button to toggle
- [ ] If toggled to OFF, display should clear after exiting config
- [ ] If toggled to ON, display should resume after exiting
- [ ] BLE indicator `B` visibility controlled by this setting

### 4.9 Config Item: Save (Item 6)
- [ ] Scroll to "Save"
- [ ] Display shows `OK` on right
- [ ] Press encoder button
- [ ] Verify confirmation screen appears:
  ```
  Settings Saved!
  ```
- [ ] Message displayed for 1 second
- [ ] Config screen returns automatically
- [ ] Check serial log

**Expected Serial Output:**
```
[CONFIG] Save configuration requested - all settings already saved
```

### 4.10 Config Item: Factory Reset (Item 7)
⚠️ **WARNING:** This will erase all settings!

- [ ] Scroll to "Factory Reset"
- [ ] Display shows `RST` on right
- [ ] Press encoder button
- [ ] Verify warning screen appears:
  ```
  Factory Reset!
  Please reboot
  ```
- [ ] Message displayed for 2 seconds
- [ ] Check serial log for confirmation
- [ ] **DO NOT REBOOT** unless you want to reset
- [ ] Power cycle device to test reset (optional)

**Expected Serial Output:**
```
[CONFIG] Factory Reset requested
[PREF] All preferences cleared - factory reset complete
```

### 4.11 Config Item: Forget (Item 8)
- [ ] Ensure device is paired with BLE client
- [ ] Scroll to "Forget"
- [ ] Display shows `BLE` on right
- [ ] Press encoder button
- [ ] Check serial log for bond clear message
- [ ] Attempt to reconnect from BLE client
- [ ] Should require re-pairing

**Expected Serial Output:**
```
[BT] Forget bonds requested
[BT] ble_store_clear() called to remove stored bonds/CCCDs
```

### 4.12 Config Item: Back (Item 9)
- [ ] Scroll to "Back"
- [ ] Display shows `<-` on right
- [ ] Press encoder button
- [ ] Verify config mode exits
- [ ] Normal operating screen should appear
- [ ] Check serial log

**Expected Serial Output:**
```
[CONFIG] Exited config mode
```

---

## Test 5: BLE Integration

### 5.1 BLE Indicator
**Prerequisites:**
- [ ] OLED indicator enabled (config item 5 = ON)
- [ ] BLE enabled (config item 2 = ON)
- [ ] BLE client ready (smartphone app)

**Test Steps:**
- [ ] Verify no `B` indicator before connection
- [ ] Connect BLE client
- [ ] Verify `B` appears on status line (top right area)
- [ ] Indicator should blink during connection animation
- [ ] Indicator steady when connected
- [ ] Disconnect BLE client
- [ ] Verify `B` disappears

### 5.2 BLE Event Messages
- [ ] Connect BLE client
- [ ] Verify "CON" message appears on status line
- [ ] Disconnect BLE client
- [ ] Verify "DIS" message appears
- [ ] Perform BLE write operation
- [ ] Verify "WR" or similar message appears
- [ ] Note: Fault messages take priority over BLE events

### 5.3 BLE Priority System
- [ ] Connect BLE device
- [ ] Verify "CON" message displayed
- [ ] Trigger sensor fault (disconnect thermocouple)
- [ ] Verify "SENSOR FAULT" replaces "CON" (higher priority)
- [ ] Clear fault
- [ ] Verify "CON" returns

---

## Test 6: OLED Reliability

### 6.1 Multi-Address Probing
**Setup:** Test with different OLED addresses

This feature is automatic. Verify in serial log:

**Expected Serial Output:**
```
[OLED] Trying address 0x3C...
[OLED] Init successful at address 0x3C
```

Or if failed:
```
[OLED] Trying address 0x3C... failed
[OLED] Trying address 0x3D...
[OLED] Init successful at address 0x3D
```

### 6.2 Pin Swap Fallback
**Setup:** Intentionally swap SDA/SCL wires

- [ ] Power off device
- [ ] Swap OLED SDA and SCL connections
- [ ] Power on device
- [ ] Verify display still works
- [ ] Check serial log for pin swap message

**Expected Serial Output:**
```
[OLED] First init failed, trying swapped pins...
[OLED] Init successful with swapped pins
```

### 6.3 Hang Detection
**Setup:** This requires monitoring serial output during normal operation

- [ ] Run device continuously for 5+ minutes
- [ ] Monitor serial output for any slow display warnings
- [ ] Normal updates should be <100ms
- [ ] No warnings expected during normal operation

If warnings appear:
```
[OLED] Display update took 523ms (possible hang), failures=1
```

This indicates I2C bus issue or display problem.

### 6.4 Auto-Recovery
**Setup:** Simulate display failure

**Method 1 - Disconnect Display:**
- [ ] Run device normally
- [ ] Disconnect OLED power or I2C lines
- [ ] Wait for 5 consecutive update failures
- [ ] Verify display disabled (check serial log)
- [ ] Wait 60+ seconds
- [ ] Reconnect display
- [ ] Verify auto-recovery attempt in serial log
- [ ] Verify display resumes operation

**Expected Serial Output:**
```
[OLED] Display update took 1523ms (possible hang), failures=1
[OLED] Display update took 1489ms (possible hang), failures=2
[OLED] Display update took 1501ms (possible hang), failures=3
[OLED] Display update took 1512ms (possible hang), failures=4
[OLED] Display update took 1498ms (possible hang), failures=5
[OLED] Too many failures, disabling display
[ERROR] OLED: Display hang detected, disabled

... 60 seconds later ...

[OLED] Attempting recovery...
[OLED] Recovery successful
```

**Method 2 - I2C Bus Jam:**
More difficult to test without special equipment.

---

## Test 7: Edge Cases

### 7.1 Rapid Mode Changes
- [ ] Rapidly press encoder button multiple times
- [ ] Alternate between menu and normal screen
- [ ] Enter config mode and exit quickly
- [ ] Verify no display corruption
- [ ] Verify no screen freeze
- [ ] Check serial log for errors

### 7.2 Simultaneous Faults
- [ ] Trigger sensor fault (disconnect thermocouple)
- [ ] Trigger battery low (if possible)
- [ ] Verify priority system works correctly
- [ ] Only highest priority fault should display
- [ ] Check error count increases for all faults
- [ ] Clear faults one by one
- [ ] Verify next priority fault displays

### 7.3 Power Loss During Update
- [ ] Rapidly power cycle device during operation
- [ ] Power on/off several times quickly
- [ ] Verify display recovers each time
- [ ] No persistent corruption
- [ ] Check serial log for recovery messages

### 7.4 Maximum Runtime
- [ ] Run continuously for 30+ minutes (or reduced threshold)
- [ ] Verify cooldown activates
- [ ] Verify display shows countdown
- [ ] Let cooldown complete
- [ ] Verify normal operation resumes
- [ ] Repeat cycle multiple times

---

## Test 8: Performance Validation

### 8.1 Update Time Monitoring
**Using Serial Monitor:**
- [ ] Enable debug logging if available
- [ ] Monitor display update time messages
- [ ] Typical updates should be 20-50ms
- [ ] Warnings if >100ms
- [ ] Failures if >500ms

### 8.2 Memory Usage
**Check during compilation:**
```
Sketch uses XXXXX bytes (XX%) of program storage space.
Global variables use XXXXX bytes (XX%) of dynamic memory.
```

Display operations should not significantly increase memory usage.

### 8.3 I2C Bus Speed
Verify I2C speed in oled.h:
```cpp
Wire.setClock(400000);  // 400kHz (fast mode)
```

- [ ] Verify fast mode (400kHz) is used
- [ ] If problems occur, try standard mode (100kHz)

---

## Test 9: Integration Verification

### 9.1 Management Commands with Display
- [ ] Send `M` command via serial (full diagnostics)
- [ ] Verify display continues updating
- [ ] Send `MS` command (status summary)
- [ ] Verify display shows current state correctly
- [ ] Send `MH` command (health check)
- [ ] Verify health status reflected on display
- [ ] Send `MR` command (reset errors)
- [ ] Verify error count clears from display

### 9.2 Temperature Display Accuracy
- [ ] Compare OLED temperature with serial output
- [ ] Both should match within 0.1°C
- [ ] Test in both °C and °F modes
- [ ] Verify conversion accuracy

### 9.3 Battery Display Accuracy
- [ ] Compare OLED battery % with serial output
- [ ] Both should match exactly
- [ ] Test with different battery levels
- [ ] Verify percentage calculation correct

### 9.4 Output Display Accuracy
- [ ] Set various manual power levels (25%, 50%, 75%, 100%)
- [ ] Verify OLED bar matches percentage
- [ ] Visual bar fill should be proportional
- [ ] Percentage number should match

---

## Test 10: Documentation Verification

### 10.1 Visual Reference Accuracy
- [ ] Compare actual display with [OLED-VISUAL-REFERENCE.md](OLED-VISUAL-REFERENCE.md)
- [ ] Verify all documented screens match reality
- [ ] Check icon positions correct
- [ ] Verify text placement accurate

### 10.2 Integration Guide Accuracy
- [ ] Review [OLED-INTEGRATION.md](OLED-INTEGRATION.md)
- [ ] Verify all features documented exist
- [ ] Check configuration actions work as documented
- [ ] Verify troubleshooting steps effective

---

## Issue Tracking

### Issues Found During Testing

| Test # | Issue Description | Severity | Status | Notes |
|--------|------------------|----------|--------|-------|
| Example | Display flickers | Low | Open | Occurs only at <10% battery |
|  |  |  |  |  |
|  |  |  |  |  |

**Severity Levels:**
- **Critical:** Display non-functional, system unusable
- **High:** Major feature broken, workaround available
- **Medium:** Minor feature issue, cosmetic problem
- **Low:** Documentation error, minor visual glitch

---

## Test Summary

### Test Results

- [ ] **PASS:** All basic display functionality tests
- [ ] **PASS:** All management feature integration tests
- [ ] **PASS:** All fault indicator tests
- [ ] **PASS:** All display mode tests
- [ ] **PASS:** All BLE integration tests
- [ ] **PASS:** All reliability tests
- [ ] **PASS:** All edge case tests
- [ ] **PASS:** All performance validation tests
- [ ] **PASS:** All integration verification tests
- [ ] **PASS:** All documentation verification tests

### Overall Assessment

**Display Functionality:** ☐ Excellent ☐ Good ☐ Acceptable ☐ Needs Work

**Management Integration:** ☐ Excellent ☐ Good ☐ Acceptable ☐ Needs Work

**Reliability:** ☐ Excellent ☐ Good ☐ Acceptable ☐ Needs Work

**User Experience:** ☐ Excellent ☐ Good ☐ Acceptable ☐ Needs Work

### Recommendations

**For Production:**
- [ ] Ready for production use
- [ ] Requires minor fixes (list below)
- [ ] Requires major fixes (list below)
- [ ] Not ready for production

**Action Items:**
1. 
2. 
3. 

### Tester Information

- **Tester Name:** _________________
- **Test Date:** ___________________
- **Firmware Version:** _____________
- **Hardware Revision:** ____________
- **Test Duration:** ________________

### Notes

Additional observations or comments:

---

## Quick Test Script (5 Minutes)

For rapid verification, perform this abbreviated test:

1. **Power On**
   - [ ] Display shows boot message
   - [ ] Normal screen appears

2. **Basic Function**
   - [ ] Press encoder - menu appears
   - [ ] Long-press encoder - config appears
   - [ ] Rotate encoder - selection moves

3. **Management**
   - [ ] Disconnect sensor - fault appears
   - [ ] Send `MR` - error clears
   - [ ] Check error count shown

4. **BLE**
   - [ ] Connect device - `B` appears
   - [ ] Disconnect - `B` disappears

5. **Serial Check**
   - [ ] Monitor serial output
   - [ ] No errors or warnings
   - [ ] Update time <100ms

**Result:** ☐ Pass ☐ Fail

If quick test passes, full testing can proceed. If fails, investigate issues before full test.
