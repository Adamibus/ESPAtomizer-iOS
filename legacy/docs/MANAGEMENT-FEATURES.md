# ESPAtomizer Management & Monitoring Features

**Version**: 2.0 (Management Enhancement Release)  
**Date**: December 30, 2025  
**Purpose**: Production-ready monitoring, diagnostics, and fail-safe mechanisms

---

## Executive Summary

The ESPAtomizer firmware has been enhanced with comprehensive management features designed for production environments, troubleshooting, and long-term reliability. These features provide:

- **Real-time health monitoring** with automatic fault detection
- **Comprehensive error logging** with source tracking and counters
- **Operational limits** (30-minute runtime, cooldown periods, absolute temperature limits)
- **Diagnostic commands** for remote troubleshooting
- **Automatic recovery** mechanisms with manual override capability

---

## 🎯 Management Features

### 1. System Health Monitoring

**Status Structure**: Real-time operational status tracking
```cpp
SystemHealth {
  operational       // Overall system OK to run
  sensorOk          // Thermocouple working
  heaterOk          // Output functional
  batteryOk         // Battery voltage acceptable
  bleOk             // BLE connected/responsive
  temperatureInRange // Temp within safe limits
  noFaults          // No active faults
  statusMessage     // Human-readable status
}
```

**Auto-Check**: Every 5 seconds during operation  
**Auto-Disable**: System automatically disables if health check fails

**Command**: `MH` - Manual health check

---

### 2. Error Logging System

**Centralized Error Tracking**:
- Total error count across all categories
- Source-specific counters (sensor, heater, battery, BLE, thermal)
- Last error timestamp and description
- Persistent error storage (survives reboot via RTC memory)

**Error Categories**:
| Category | Description | Auto-Action |
|----------|-------------|-------------|
| SENSOR | Thermocouple read failures | Disable heater after 5 consecutive |
| HEATER | Output control failures | Log and disable |
| BATTERY | Voltage anomalies, charger faults | Ramp down output |
| BLE | Communication failures | Log only |
| THERMAL | Runaway detection | Immediate disable |
| RUNTIME | Max runtime exceeded | Disable + cooldown |

**Function**: `logError(source, message)`  
**Example**: `logError("SENSOR", "Consecutive bad reads threshold reached");`

**Command**: `M` - View all error logs

---

### 3. Operational Safety Limits

#### Absolute Temperature Limits
- **Max**: 350°C (hard limit, cannot be exceeded)
- **Min**: 0°C (prevents negative temperature readings)
- **Action**: Immediate shutdown if exceeded

#### Continuous Runtime Limit
- **Duration**: 30 minutes maximum continuous operation
- **Cooldown**: 1 minute mandatory cooldown after max runtime
- **Purpose**: Prevent overheating of heating element and electronics
- **Override**: Must wait for cooldown to complete before re-enabling

#### Error Count Threshold
- **Limit**: 10 total errors before requiring manual reset
- **Action**: Auto-disable system when threshold reached
- **Recovery**: Use `MR` command to reset counters

---

### 4. Diagnostic Counters

**Tracked Metrics**:
```
loopIterations      // Total loop() executions
pidComputations     // PID calculate() calls
outputChanges       // PWM duty changes
bleWrites           // BLE characteristic writes
sensorReads         // Temperature read attempts
sensorReadsFailed   // Failed sensor reads
```

**Purpose**: Performance analysis, troubleshooting, identifying bottlenecks

**Command**: `M` - Full diagnostics report with all counters

---

### 5. Management Serial Commands

#### `M` - Full Diagnostics Report
Displays:
- Uptime in seconds
- All diagnostic counters
- Complete error log (all categories with counts)
- System health status (all flags)
- Boot self-test results
- Current operational status (temp, setpoint, PID, battery)
- Safety status (faults, cooldown state)

**Example Output**:
```
===== SYSTEM DIAGNOSTICS =====
Uptime: 3600 seconds
Loop Iterations: 360000
PID Computations: 3600
Output Changes: 450
Sensor Reads: 3600 (Failed: 12, 0.3%)
BLE Writes: 145

----- Error Log -----
Total Errors: 2
  Sensor: 1
  Heater: 0
  Battery: 1
  BLE: 0
  Thermal Runaway: 0
Last Error: [BATTERY] Low voltage warning
Last Error Time: 120 sec ago

----- System Health -----
Operational: YES
Sensor OK: YES
Heater OK: YES
Battery OK: YES
BLE OK: YES
Temp in Range: YES
No Faults: NO
Status: Low battery warning
Last Health Check: 3 sec ago

----- Boot Self-Test Results -----
Battery: PASS
Sensor: PASS
Heater: PASS
Encoder: PASS
Button: PASS

----- Current Status -----
System Enabled: YES
Mode: AUTO
Manual Mode: NO
Temperature: 245.3 C
Setpoint: 250.0 C
PID Output: 650.0 (63.5%)
Kp=10.000 Ki=0.500 Kd=50.000
Battery: 3.85V (75%)

----- Safety Status -----
Sensor Faulted: NO
Thermal Runaway: NO
Watchdog Faulted: NO
Charger Removed Fault: NO
Cooldown Required: NO
=============================
```

#### `MR` - Reset Error Counters & Faults
- Clears all error counters (resets to 0)
- Clears all fault states (sensor, thermal, watchdog, charger)
- Resets health status to "System OK"
- Clears persistent boot error message
- **Use**: After fixing issues to resume operation

#### `MH` - Manual Health Check
- Immediately runs system health assessment
- Updates all status flags
- Displays current health status message
- **Use**: Verify system status before critical operation

#### `MS` - System Status Summary
Quick summary of critical status:
```
===== SYSTEM STATUS SUMMARY =====
Operational: YES
Status: System OK
Errors: 0
Enabled: YES
Mode: AUTO
Temp: 245.3°C
Setpoint: 250.0°C
Output: 650/1023 (63.5%)
=================================
```

---

## 🔧 Integration with Existing Features

### Boot Self-Tests
All self-test results now tracked:
- Battery voltage check
- Sensor connectivity test
- Heater output test
- Encoder functionality
- Button responsiveness

**Access**: `T` command shows all test results

### Sensor Fault Detection
Enhanced with error logging:
- Consecutive bad reads trigger `logError("SENSOR", ...)`
- Failed read counter increments on every failure
- Success rate calculation for diagnostics

### Thermal Runaway Protection
Enhanced with error logging:
- All 3 thermal runaway conditions log errors
- Logged as "THERMAL" category for tracking
- Persistent error message survives reboot

### Battery Management
Enhanced with error logging:
- Low voltage warnings logged
- Charger removal faults logged
- Battery errors tracked separately

---

## 📊 Monitoring Best Practices

### Pre-Production Testing
1. Run `Y` (smoke test) after firmware flash
2. Verify all boot tests pass: `T` command
3. Check baseline diagnostics: `M` command
4. Enable system and monitor for 5 minutes
5. Run `MS` to verify operational status

### Daily Operations
1. Check system health: `MH` every shift
2. Review error log: `M` if any issues observed
3. Monitor sensor read success rate (should be >99%)
4. Track runtime to avoid 30-minute limit

### Troubleshooting Workflow
1. Observe fault symptoms (heater disabled, BLE disconnects, etc.)
2. Run `M` to get full diagnostics
3. Check error log for recent errors
4. Review system health flags to identify failed component
5. Fix root cause (sensor connection, battery, etc.)
6. Reset error counters: `MR`
7. Re-enable system and monitor: `MH` + `MS`

### Remote Monitoring Integration
Serial commands can be automated via USB/UART:
```python
# Python example for remote monitoring
import serial

ser = serial.Serial('/dev/ttyUSB0', 115200)

# Get system status every minute
while True:
    ser.write(b'MS\n')  # System status summary
    response = ser.read_until(b'=====\n')
    log_to_database(response)
    time.sleep(60)
```

---

## 🚨 Error Recovery Procedures

### "SENSOR FAULT"
**Cause**: 5+ consecutive thermocouple read failures  
**Actions**:
1. Check thermocouple connection
2. Verify I2C wiring (SDA/SCL)
3. Check ADS1115 power supply
4. Run `T` to test sensor
5. Reset: `MR` + re-enable

### "THERMAL RUNAWAY"
**Cause**: Temperature exceeds limits or PID divergence  
**Actions**:
1. Immediately disconnect power (safety)
2. Inspect heating element for shorts
3. Verify thermocouple placement
4. Check PID tunings (may be too aggressive)
5. Review diagnostics: `M`
6. Reset: `MR` after fixing hardware

### "Max Runtime Exceeded"
**Cause**: 30 minutes continuous operation  
**Actions**:
1. Wait 1 minute for cooldown
2. System auto-disables (safety feature)
3. Allow device to cool down
4. Check `MS` after cooldown expires
5. Re-enable normally (no reset needed)

### "Too Many Errors"
**Cause**: 10+ total errors accumulated  
**Actions**:
1. Run `M` to identify error sources
2. Address most frequent error category first
3. Fix underlying issues
4. Reset counters: `MR`
5. Monitor closely: `MH` every 30 seconds

---

## 🔒 Safety Features Summary

| Feature | Threshold | Action | Recovery |
|---------|-----------|--------|----------|
| Absolute Max Temp | 350°C | Immediate disable | Fix + MR |
| Thermal Runaway | Setpoint + 50°C | Disable heater | Fix + MR |
| Sensor Fault | 5 bad reads | Disable heater | Check sensor + MR |
| Max Runtime | 30 minutes | Auto-disable + cooldown | Wait 1 min |
| Error Threshold | 10 total errors | Disable system | Fix + MR |
| Battery Low | < 3.3V | Disable heater | Charge battery |
| Cooldown | After max runtime | 60 seconds | Automatic |

---

## 📈 Performance Metrics

### Expected Values (Normal Operation)
- **Sensor Read Success**: >99%
- **Loop Iterations**: ~1000/second
- **PID Computations**: ~1/second
- **Error Rate**: <1 error per hour
- **Health Checks**: Pass rate 100%

### Alert Thresholds
- **Sensor Failures**: >5% fail rate
- **Errors**: >5 in 10 minutes
- **Temperature**: >90% of max continuously
- **Runtime**: >25 minutes (approaching limit)

---

## 🎓 Training Guide for Operators

### Quick Reference Card
```
Management Commands:
 M   = Full diagnostics (use when troubleshooting)
 MR  = Reset errors (use after fixing problems)
 MH  = Health check (use before critical operations)
 MS  = Status summary (use for quick status)
 H   = Help (list all commands)

Safety Limits:
 Max Temp: 350°C (absolute)
 Max Runtime: 30 min (then 1 min cooldown)
 Max Errors: 10 (then auto-disable)

Normal Operation:
 1. Power on device
 2. Run MH (health check)
 3. Enable system (button or BLE)
 4. Monitor with MS every 5-10 minutes
 5. Run M if any issues
```

### Common Scenarios
1. **Device won't heat**: Run `MS`, check "Operational" flag
2. **Erratic temperature**: Run `T`, check sensor status
3. **Unexpected shutdown**: Run `M`, review error log
4. **After maintenance**: Run `Y` (smoke test), then `MH`

---

## 📝 Changelog (Management Features)

### Version 2.0 (December 30, 2025)
**Added**:
- SystemHealth structure with 7 status flags
- ErrorLog structure with category-specific counters
- Operational limits (30min runtime, 1min cooldown)
- Diagnostic counters (loop iterations, sensor reads, etc.)
- logError() function for centralized error tracking
- checkSystemHealth() with automatic fault detection
- printDiagnostics() for comprehensive status reporting
- resetErrorCounters() for manual recovery
- Serial commands: M, MR, MH, MS
- Automatic system disable on health check failure
- Runtime limit enforcement with cooldown period
- Error threshold (10 errors = auto-disable)

**Enhanced**:
- Sensor fault detection now logs errors
- Thermal runaway now logs all 3 conditions
- Battery monitoring logs voltage anomalies
- Help command includes management section

---

## 📞 Support & Troubleshooting

For issues not covered in this guide:
1. Capture full diagnostics output (`M` command)
2. Note last 10 error messages from serial log
3. Record temperature, setpoint, and output at time of issue
4. Check [FIRMWARE-BLE-AUDIT.md](FIRMWARE-BLE-AUDIT.md) for known issues
5. Refer to [BRINGUP.md](BRINGUP.md) for hardware validation

---

**Document Version**: 1.0  
**Firmware Compatibility**: ESPAtomizer v0.1+  
**Last Updated**: December 30, 2025
