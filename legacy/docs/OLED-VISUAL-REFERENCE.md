# OLED Display Visual Reference

## Screen Layouts

### Normal Operating Display - All Good
```
┌─────────────────────────────┐
│PWR:ON [AUTO]        BAT:85% │  ← Status line
│CON                          │  ← BLE event
│                             │
│245.3C                       │  ← Temperature (large)
│                             │
│SP:250.0C                    │  ← Setpoint (large)
│                             │
│                        63%  │  ← Output percentage
│┌───────────────────────────┐│
││========>                  ││  ← Output bar
│└───────────────────────────┘│
└─────────────────────────────┘
```

### Normal Display - With Errors
```
┌─────────────────────────────┐
│PWR:ON [AUTO] E:3    BAT:85% │  ← Error count shown
│SENSOR FAULT                 │  ← Priority fault message
│                             │
│---                          │  ← No temperature (faulted)
│                             │
│SP:250.0C                    │
│                             │
│                        63% !│  ← Runtime warning icon
│┌───────────────────────────┐│
││/\\/\\/\\/\\/\\/\\/\\/\\/\\/│  ← Warning stripes
│└───────────────────────────┘│
└─────────────────────────────┘
```

### Normal Display - System Halted
```
┌─────────────────────────────┐
│PWR:OFF [AUTO] !ERR  BAT:42% │  ← System halted indicator
│Too many errors: halted     │  ← Health status message
│                             │
│---                          │
│                             │
│SP:250.0C                    │
│                             │
│                         0%  │
│┌───────────────────────────┐│
││                           ││  ← Empty output bar
│└───────────────────────────┘│
└─────────────────────────────┘
```

### Normal Display - Cooldown Mode
```
┌─────────────────────────────┐
│PWR:ON [AUTO]        BAT:67% │
│COOLDOWN: 45s                │  ← Cooldown timer (priority)
│                             │
│87.2C                        │  ← Temperature cooling down
│                             │
│SP:250.0C                    │
│                             │
│                         0%  │  ← Output disabled
│┌───────────────────────────┐│
││                           ││
│└───────────────────────────┘│
└─────────────────────────────┘
```

### Normal Display - Battery Low
```
┌─────────────────────────────┐
│PWR:ON [AUTO] E:1    BAT:15% │
│BATTERY LOW                  │  ← Battery fault message
│                             │
│245.3C                       │
│                             │
│SP:250.0C                    │
│                             │
│                    ● 63%    │  ← Battery icon (filled circle)
│┌───────────────────────────┐│
││========>                  ││
│└───────────────────────────┘│
└─────────────────────────────┘
```

### Normal Display - Manual Mode
```
┌─────────────────────────────┐
│PWR:ON [MAN]         BAT:92% │  ← Manual mode indicator
│DIS                          │
│                             │
│245.3C                       │  ← Temperature shown
│                             │
│                             │  ← No setpoint (manual mode)
│                             │
│                        75%  │  ← Manual output level
│┌───────────────────────────┐│
││===========>               ││
│└───────────────────────────┘│
└─────────────────────────────┘
```

### Config Mode Display
```
┌─────────────────────────────┐
│  ┌─────────────────────────┐│
│  │ Default SP      250.0C  ││  ← Menu items
│  │ Unit                 C  ││
│  │►BLE ON              ON  ││  ← Selected item (inverted)
│  │ Name              Dev1  ││
│  │ AdvInt           200ms  ││
│  │ OLED               ON   ││
│  │ Save               OK   ││
│  │ Factory Reset     RST   ││
│▲ └─────────────────────────┘│  ← Scroll up arrow
│▼                            │  ← Scroll down arrow
└─────────────────────────────┘
```

### Config Mode - Save Confirmation
```
┌─────────────────────────────┐
│                             │
│                             │
│                             │
│   Settings Saved!           │  ← Confirmation message
│                             │  (shown for 1 second)
│                             │
│                             │
│                             │
│                             │
│                             │
└─────────────────────────────┘
```

### Config Mode - Factory Reset Warning
```
┌─────────────────────────────┐
│                             │
│  Factory Reset!             │  ← Warning message
│                             │
│                             │
│  Please reboot              │  ← Instructions
│                             │  (shown for 2 seconds)
│                             │
│                             │
│                             │
│                             │
└─────────────────────────────┘
```

### Menu Overlay Display
```
┌─────────────────────────────┐
│     ┌─────────────────┐     │
│     │►OFF            │     │  ← Selected mode (inverted)
│     │ Hold 100%       │     │
│     │ Hold  75%       │     │
│     │ Hold  50%       │     │
│     │ Hold  25%       │     │
│     │ Auto Normal     │     │
│     │ Auto Aggressive │     │
│     │ Auto Conserv... │     │
│     └─────────────────┘     │
│                             │
└─────────────────────────────┘
```

## Icon Legend

### Status Line Indicators
- `PWR:ON` / `PWR:OFF` - System power state
- `[AUTO]` - Automatic PID mode
- `[MAN]` - Manual output mode
- `E:X` - Error count (X = number of errors)
- `!ERR` - System halted due to errors
- `B` - BLE connected (when OLED indicator enabled)
- `BAT:XX%` - Battery percentage

### Warning Icons
- `!` - Runtime warning (>25 minutes continuous operation)
- `●` - Battery low warning (filled circle)
- `▲` - More items above (scroll indicator)
- `▼` - More items below (scroll indicator)

### Visual Patterns
- `/\/\/\` - Diagonal warning stripes (fault active on output bar)
- `►` - Selected menu item indicator
- Inverted colors - Currently selected/highlighted item
- Border rectangle - Menu boundaries

## Display Priority System

The status line (second row) shows messages in priority order:

**Priority 1 (Highest):** Cooldown timer
- Format: `COOLDOWN: XXs`
- Shown during mandatory cooldown period
- Blocks all other messages

**Priority 2:** Sensor fault
- Message: `SENSOR FAULT`
- Thermocouple disconnected or failed
- Temperature shows `---`

**Priority 3:** Thermal runaway
- Message: `THERMAL FAULT`
- Temperature rising too fast or out of control
- Warning stripes added to output bar

**Priority 4:** Battery low
- Message: `BATTERY LOW`
- Battery voltage below threshold
- Filled circle icon shown

**Priority 5:** System health
- Shows `systemHealth.statusMessage`
- Only when system not operational
- Various system-level issues

**Priority 6 (Lowest):** BLE events
- Messages: `CON`, `DIS`, `WR`, etc.
- Connection/disconnection notifications
- Only shown when no faults present

## Display Update Timing

- **Update Frequency:** Once per loop iteration (~100-200ms)
- **Normal Update Time:** 20-50ms
- **Warning Threshold:** >100ms (logged)
- **Critical Threshold:** >500ms (failure counted)
- **Failure Limit:** 5 consecutive failures
- **Recovery Interval:** 60 seconds after disable

## Screen Dimensions

- **Total Display:** 128 x 64 pixels
- **Text Size 1:** 6 x 8 pixels per character
- **Text Size 2:** 12 x 16 pixels per character
- **Output Bar:** Full width (minus margins), 6 pixels high
- **Config Menu:** Scrollable window, ~10 pixels per item
- **Menu Overlay:** Centered, ~70% of display width

## Character Limits

- **Status Line (size 1):** ~21 characters per line
- **Temperature (size 2):** ~10 characters
- **Setpoint (size 2):** ~10 characters
- **Config Item Name:** ~15 characters
- **Config Item Value:** ~8 characters

## Power Consumption

Typical OLED power usage:
- **All pixels off:** ~5mA
- **Typical display (30% pixels):** ~15-20mA
- **All pixels on:** ~30mA
- **Display off (disabled):** ~0.1mA

Note: Can disable OLED indicator (config item 5) to save power when BLE connected.

## Display States Summary

| State | Power | Mode | Errors | Status Line | Temperature | Output Bar |
|-------|-------|------|--------|-------------|-------------|------------|
| Normal Operation | ON | AUTO | 0 | BLE events | Actual temp | Filled |
| With Errors | ON | AUTO | >0 | Fault message | Actual temp | Striped |
| System Halted | OFF | AUTO/MAN | >10 | Health message | --- | Empty |
| Cooldown | ON | AUTO | Any | Countdown | Cooling temp | Empty |
| Manual Mode | ON | MAN | 0 | BLE events | Actual temp | Manual level |
| Config Mode | Any | Any | Any | N/A (menu) | N/A | N/A |
| Menu Active | Any | Any | Any | N/A (overlay) | N/A | N/A |

## Common Display Scenarios

### Scenario 1: Normal Heating Session
```
PWR:ON [AUTO]        BAT:100%
CON
245.3C → 248.7C → 249.5C → 250.1C
SP:250.0C
Output: 100% → 75% → 45% → 25%
```

### Scenario 2: Sensor Disconnected
```
PWR:ON [AUTO] E:1     BAT:95%
SENSOR FAULT
---
SP:250.0C
Output: 0% (disabled)
Warning stripes active
```

### Scenario 3: Approaching Runtime Limit
```
PWR:ON [AUTO]         BAT:68%
CON
245.3C
SP:250.0C
Output: 63% !  ← Warning icon at 25+ min
```

### Scenario 4: Mandatory Cooldown
```
PWR:ON [AUTO]         BAT:65%
COOLDOWN: 45s  ← Priority message
87.2C  ← Temperature dropping
SP:250.0C
Output: 0%  ← Heater disabled
```

### Scenario 5: Error Threshold Exceeded
```
PWR:OFF [AUTO] !ERR   BAT:60%
Too many errors: halted
---
SP:250.0C
Output: 0%
System requires reset (MR command)
```

### Scenario 6: Battery Critical
```
PWR:ON [AUTO] E:2     BAT:12%
BATTERY LOW
245.3C
SP:250.0C
Output: ● 50%  ← Battery warning icon
```

### Scenario 7: Configuration Changes
```
Config Menu:
  Default SP    250.0C
► Unit              C  ← Change to F
  BLE ON         ON
  Save           OK   ← Press to save
```

### Scenario 8: Factory Reset
```
Factory Reset!
Please reboot

(System will restore defaults on next boot)
```

## Integration with Management Features

### Error Logging Integration
- `E:X` shown when `errorLog.totalErrors > 0`
- `!ERR` shown when `systemHealth.operational == false`
- Error count updates in real-time
- Cleared by `MR` (reset) command

### Health Monitoring Integration
- Status message from `systemHealth.statusMessage`
- Displayed when system not operational
- Updates every 5 seconds via `checkSystemHealth()`
- Visible confirmation of system state

### Safety Limits Integration
- Cooldown timer shows exact remaining time
- Runtime warning at 25 minutes (before 30-min limit)
- Visual feedback prevents user confusion
- Automatic system protection visible to user

### Diagnostic Counter Integration
- No direct display (use serial `M` command)
- Error counts visible on OLED
- System health reflected in display state
- Management features complement OLED feedback

## Accessibility Features

### Visual Indicators
- High contrast (white on black)
- Large text for temperature (size 2)
- Icons supplement text messages
- Warning patterns (stripes) for immediate recognition

### User Feedback
- Immediate visual response to actions
- Confirmation messages for critical operations
- Clear error messages (not just codes)
- Status always visible during operation

### Fail-Safe Design
- System continues operation if display fails
- Serial logging always available
- Auto-recovery attempts
- Management commands work without display

## Summary

The OLED display provides **comprehensive visual feedback** for:
- ✅ System status (power, mode, battery)
- ✅ Error and health monitoring
- ✅ Fault conditions with priority system
- ✅ Real-time temperature and output
- ✅ Configuration interface
- ✅ Safety warnings (runtime, cooldown, battery)
- ✅ User action confirmation

All management features are **fully integrated** and provide immediate visual feedback to the user, making the device easy to monitor and troubleshoot.
