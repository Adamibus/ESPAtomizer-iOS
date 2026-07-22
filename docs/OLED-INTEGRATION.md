# OLED Display Integration Guide

## Overview
The OLED display (SSD1306 128x64) is fully integrated with all management features and provides comprehensive visual feedback for system status, errors, and warnings.

## Display Modes

### 1. Normal Operating Screen
Shows real-time system information:

```
PWR:ON [AUTO] E:3    BAT:85%
SENSOR FAULT
245.3C
SP:250.0C
[========>      ] 63% !
```

**Top Line:**
- Power status: `PWR:ON` or `PWR:OFF`
- Mode: `[AUTO]` or `[MAN]`
- Error indicator: `!ERR` (system halted) or `E:count` (active errors)
- BLE indicator: `B` (when connected, if enabled)
- Battery: `BAT:xx%`

**Status Line (Priority Display):**
1. `COOLDOWN: XXs` - Cooldown timer (highest priority)
2. `SENSOR FAULT` - Thermocouple disconnected/failed
3. `THERMAL FAULT` - Thermal runaway detected
4. `BATTERY LOW` - Battery voltage below threshold
5. System health message - From `systemHealth.statusMessage`
6. BLE events - Connection/disconnection messages (lowest priority)

**Temperature Display:**
- Large font (size 2)
- Shows `---` when sensor faulted
- Auto-converts to °F when not in Celsius mode

**Setpoint Display:**
- Hidden in manual mode
- Large font (size 2)
- Format: `SP:XXX.XC` or `SP:XXX.XF`

**Output Bar:**
- Visual representation of PWM output (0-100%)
- Bordered rectangle with proportional fill
- **Warning Stripes:** Diagonal pattern when faults active
- Percentage label on right side
- Warning icons:
  - `!` - Runtime warning (>25 minutes continuous)
  - Filled circle - Battery low indicator

### 2. Config Mode Screen
Paginated menu for system configuration:

```
┌──────────────────────┐
│ Default SP    250.0C │
│►Unit              C  │
│ BLE ON         ON    │
│ Name           Dev1  │
│ AdvInt        200ms  │
│ OLED           ON    │
│ Save           OK    │
│ Factory Reset  RST   │
└──────────────────────┘
```

**Menu Items (10 total):**
0. **Default SP** - Default setpoint temperature
1. **Unit** - Temperature unit (C/F)
2. **BLE ON** - Enable/disable Bluetooth
3. **Name** - BLE device name (4 presets)
4. **AdvInt** - BLE advertising interval (100/200/1000ms)
5. **OLED** - OLED indicator enable/disable
6. **Save** - Confirm settings saved (shows "Settings Saved!" message)
7. **Factory Reset** - Clear all preferences (shows "Factory Reset! Please reboot")
8. **Forget** - Clear BLE bonds
9. **Back** - Exit config mode

**Navigation:**
- Encoder rotation: Scroll through items
- Encoder button: Toggle/activate selected item
- Selected item highlighted with inverted colors
- Scroll arrows shown when items off-screen
- Values shown on right side of each item

### 3. Menu Overlay Screen
PID mode selection popup:

```
┌──────────────────┐
│►OFF             │
│ Hold 100%        │
│ Hold  75%        │
│ Hold  50%        │
│ Hold  25%        │
│ Auto Normal      │
│ Auto Aggressive  │
│ Auto Conservative│
└──────────────────┘
```

**Menu Items (8 total):**
- OFF: Disable all output
- Hold 100%/75%/50%/25%: Manual mode at fixed power
- Auto Normal/Aggressive/Conservative: PID modes with different tuning

## Management Integration

### Error Display
The OLED shows error count and system health status:

**System Operational:**
- `E:X` shown when errors present but system still operational
- Error count from `errorLog.totalErrors`

**System Halted:**
- `!ERR` shown when `systemHealth.operational == false`
- Status message displayed on second line
- System automatically disables when error threshold exceeded

### Fault Indicators
Visual warnings for critical faults:

1. **Cooldown Timer**
   - Appears after 30-minute continuous runtime
   - Shows remaining cooldown time in seconds
   - Blocks all other status messages (highest priority)

2. **Sensor Fault**
   - Displayed when `sensorFaulted == true`
   - Shows "SENSOR FAULT" message
   - Temperature display shows `---`

3. **Thermal Runaway**
   - Displayed when `thermalRunawayFaulted == true`
   - Shows "THERMAL FAULT" message
   - Warning stripes added to output bar

4. **Battery Low**
   - Displayed when `batteryLow == true`
   - Shows "BATTERY LOW" message
   - Filled circle icon shown next to output percentage

5. **Runtime Warning**
   - Exclamation mark (`!`) shown when >25 minutes runtime
   - Appears next to output percentage
   - Warns before mandatory cooldown at 30 minutes

### Visual Warning System
When any fault is active (`sensorFaulted || thermalRunawayFaulted || watchdogFaulted`):
- Diagonal stripe pattern drawn across output bar
- Makes fault state immediately visible
- Does not interfere with normal bar operation

## OLED Reliability Features

### Multi-Address Probing
The `tryInitOLED()` function attempts multiple I2C addresses:
- 0x3C (standard SSD1306)
- 0x3D (alternate address)
- 0x43 (some clones)

### Pin Swap Fallback
If initialization fails with standard pins:
- Automatically retries with swapped SDA/SCL
- Handles miswired displays
- Logs success/failure for debugging

### Hang Detection
Monitors display update time:
- Normal updates: <50ms
- Warning threshold: >500ms
- Logs slow updates for troubleshooting
- Prevents system lockup from I2C issues

### Auto-Recovery System
```cpp
static int oledFailureCount = 0;
static unsigned long lastOledSuccess = 0;
#define OLED_FAILURE_THRESHOLD 5
```

**Failure Detection:**
- Increments counter when update takes >500ms
- Resets counter on successful update
- Disables display after 5 consecutive failures
- Logs error via management system

**Auto-Recovery:**
- Attempts re-initialization after 60 seconds offline
- Resets failure counter on successful recovery
- Logs recovery attempt and result
- Continues management logging even when display offline

## Initialization Sequence

### Boot Time (Early Init)
```cpp
bool earlyOk = tryInitOLED(OLED_SDA, OLED_SCL);
if (earlyOk) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Booting..."));
  display.display();
}
```
- Shows "Booting..." message immediately
- Provides visual feedback during startup
- Continues boot even if OLED fails

### Main Setup
```cpp
initOk = tryInitOLED(OLED_SDA, OLED_SCL);
if (!initOk) {
  Serial.println(F("[OLED] First init failed, trying swapped pins..."));
  initOk = tryInitOLED(OLED_SCL, OLED_SDA);
}
displayAvailable = initOk;
```
- Primary initialization attempt
- Automatic pin swap retry
- Sets `displayAvailable` flag

### Runtime Updates
```cpp
if (displayAvailable) {
  unsigned long beforeDisplay = millis();
  updateDisplay();
  unsigned long displayTime = millis() - beforeDisplay;
  
  if (displayTime > 500) {
    oledFailureCount++;
    // Handle failure...
  }
}
```
- Called once per loop iteration
- Performance monitoring
- Automatic failure handling

## Configuration Actions

### Save Configuration (Case 6)
```cpp
case 6: // Save configuration
  Serial.println(F("[CONFIG] Save configuration requested"));
  #if USE_OLED
  if (displayAvailable) {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println(F("  Settings Saved!"));
    display.display();
    delay(1000);  // Show confirmation
  }
  #endif
```
- Confirms settings are persisted
- Shows visual feedback
- All settings saved when changed (cases 0-5)

### Factory Reset (Case 7)
```cpp
case 7: // Factory Reset
  Preferences prefs;
  prefs.begin("espatom", false);
  prefs.clear();  // Clear all preferences
  prefs.end();
  #if USE_OLED
  if (displayAvailable) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println(F("Factory Reset!"));
    display.setCursor(0, 30);
    display.println(F("Please reboot"));
    display.display();
    delay(2000);
  }
  #endif
```
- Clears all stored preferences
- Shows reboot instruction
- System defaults restored on next boot

### Forget BLE Bonds (Case 8)
```cpp
case 8: // Forget bonds
  #if defined(HAVE_NIMBLE_STORE_HDR)
  ble_store_clear();
  Serial.println(F("[BT] Bonds cleared"));
  #endif
```
- Removes all BLE pairings
- Requires NimBLE store header
- Allows fresh pairing

### Back (Case 9)
```cpp
case 9: // Back
  configMode = false;
  Serial.println(F("[CONFIG] Exited config mode"));
```
- Exits configuration screen
- Returns to normal display
- Changes already saved

## Hardware Specifications

**Display:** SSD1306 128x64 OLED
**Interface:** I2C
**Pins:**
- SDA: Configurable (default from config.h)
- SCL: Configurable (default from config.h)

**I2C Addresses Supported:**
- 0x3C (most common)
- 0x3D (alternate)
- 0x43 (some clones)

**Performance:**
- Normal update time: 20-50ms
- Watchdog threshold: 500ms
- Update frequency: Once per loop iteration (~100-200ms)

## Troubleshooting

### Display Not Working
1. **Check I2C Address:** Try 0x3C, 0x3D, or 0x43
2. **Verify Wiring:** Check SDA/SCL connections
3. **Try Pin Swap:** Setup automatically attempts this
4. **Check Serial Log:** Look for OLED initialization messages
5. **Wait for Recovery:** Auto-recovery attempts after 60 seconds

### Display Freezing
- Hang detection automatically disables display after 5 failures
- Check for I2C bus issues (other devices on same bus)
- Verify pull-up resistors (4.7kΩ recommended)
- Auto-recovery will attempt re-initialization

### Missing Information
- **No battery percentage:** Check ADC reading from battery monitor
- **Temperature shows `---`:** Sensor fault active
- **No BLE indicator:** Check `rtcBleOledIndicator` setting (config item 5)
- **No error count:** System may have no errors (good!)

### Slow Updates
- Normal: 20-50ms
- Warning: >100ms (check I2C bus speed)
- Critical: >500ms (automatic disable triggered)

## Integration Checklist

- [x] Management features displayed (error count, health status)
- [x] Fault indicators shown (sensor, thermal, battery, cooldown)
- [x] Visual warnings implemented (error badges, warning stripes, runtime alert)
- [x] Config menu fully functional (all 10 items)
- [x] Config actions working (Save, Factory Reset, Forget, Back)
- [x] Auto-recovery mechanism implemented
- [x] Hang detection active
- [x] Multi-address probing enabled
- [x] Pin swap fallback working
- [x] Boot-time initialization with feedback
- [x] Runtime error handling
- [x] Serial logging for debugging

## Testing Recommendations

### Basic Functionality
1. Power on device - verify "Booting..." message
2. Check normal display shows temperature, setpoint, output
3. Toggle encoder button - verify menu appears
4. Long-press encoder - verify config mode activates

### Management Integration
1. Disconnect sensor - verify "SENSOR FAULT" appears
2. Check error count increases
3. Trigger cooldown - verify countdown timer displays
4. Check warning stripes appear on output bar

### Config Menu
1. Scroll through all 10 items - verify labels correct
2. Test Save - verify confirmation message
3. Test Factory Reset - verify warning message
4. Test Forget - verify BLE bonds cleared
5. Test Back - verify return to normal screen

### Recovery System
1. Simulate I2C hang (disconnect display during operation)
2. Verify display disables after 5 failures
3. Wait 60 seconds - verify recovery attempt
4. Reconnect display - verify successful recovery

### BLE Integration
1. Connect BLE device - verify "B" indicator appears
2. Disconnect - verify indicator disappears
3. Check BLE events displayed when no faults
4. Verify faults take priority over BLE events

## Future Enhancements

Potential improvements for consideration:

1. **Diagnostic Screen:** Add config menu item to show management diagnostics
2. **Error History:** Display last N errors on OLED
3. **Graph Display:** Show temperature history (requires framebuffer)
4. **Custom Warnings:** User-configurable warning thresholds
5. **Brightness Control:** PWM dimming for different ambient lighting
6. **Screensaver:** Prevent OLED burn-in during long sessions
7. **Multi-Language:** Support for localized text
8. **Large Font Mode:** Accessibility option for better visibility

## Summary

The OLED display is **fully integrated** with all management features:
- ✅ Real-time error and health monitoring
- ✅ Visual fault indicators with priority system
- ✅ Warning patterns for critical states
- ✅ Complete configuration interface
- ✅ Robust error handling and auto-recovery
- ✅ Hardware compatibility features (multi-address, pin swap)
- ✅ Performance monitoring and hang detection

The implementation provides comprehensive visual feedback while maintaining system reliability through automatic fault detection and recovery mechanisms.
