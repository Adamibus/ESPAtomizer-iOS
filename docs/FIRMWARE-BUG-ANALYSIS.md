# ESPAtomizer Firmware - Comprehensive Bug & Accuracy Analysis

**Date:** December 30, 2025  
**Firmware Version:** ESPAtomizer v0.1  
**Files Analyzed:** 3,023 lines (main .ino) + 13 header/implementation files  
**Analysis Type:** Deep static code review, logic verification, safety audit

---

## Executive Summary

| Severity | Count | Status |
|----------|-------|--------|
| 🔴 **CRITICAL** | 4 | Requires immediate fix |
| 🟠 **HIGH** | 8 | Should fix before production |
| 🟡 **MEDIUM** | 12 | Fix during next maintenance |
| 🟢 **LOW** | 15 | Optional improvements |
| **TOTAL ISSUES** | **39** | **Mixed severity** |

**Overall Code Quality:** 7.5/10 - Good foundation with safety features, but contains logic bugs and potential hazards

---

## 🔴 CRITICAL BUGS (Immediate Action Required)

### BUG #1: Battery Low Warning Logic Error (Line 1245)
**Severity:** 🔴 CRITICAL  
**File:** ESPAtomizer.ino  
**Location:** Lines 1241-1248  

**Issue:** Impossible condition creates dead code
```cpp
} else if (batteryVoltage < BAT_LOW_WARNING_V) {
  // Warning state: voltage below 3.5V
  if (!batteryHysteresisActive) {
    batteryLowWarning = true;
    if (!batteryLowWarning) {  // ❌ IMPOSSIBLE - just set to true above!
      Serial.printf("[BAT] WARNING: Voltage %.2fV < threshold %.2fV, disabling AUTO mode\n", 
                    batteryVoltage, BAT_LOW_WARNING_V);
    }
  }
  batteryHysteresisActive = true;
}
```

**Impact:** Warning message never prints, user never informed of low battery  
**Root Cause:** Copy-paste error or logic inversion mistake  
**Fix:**
```cpp
} else if (batteryVoltage < BAT_LOW_WARNING_V) {
  if (!batteryHysteresisActive) {
    if (!batteryLowWarning) {  // Only print if transitioning to warning state
      Serial.printf("[BAT] WARNING: Voltage %.2fV < threshold %.2fV\n", 
                    batteryVoltage, BAT_LOW_WARNING_V);
    }
    batteryLowWarning = true;
  }
  batteryHysteresisActive = true;
}
```

---

### BUG #2: Unsigned Long Underflow in Time Calculations
**Severity:** 🔴 CRITICAL  
**File:** ESPAtomizer.ino  
**Multiple Locations:** Lines 1354, 1397, 1412, 1700, 1723, 2065, 2087, 2283, 2284, 2287, etc.

**Issue:** Unsigned arithmetic underflows when `now < startTime` (millis() overflow at ~49 days)
```cpp
unsigned long held = now - btnPressStartMs;  // ❌ Underflows if millis() wraps
unsigned long remaining = (COOLDOWN_REQUIRED_MS - (now - cooldownStartMs)) / 1000;  // ❌ Underflow
unsigned long loopElapsed = now - watchdogLastLoopMs;  // ❌ Dangerous for safety checks
```

**Impact:**
- Watchdog may never trigger (thinks loop is fast when it's actually stalled)
- Button timing becomes incorrect after ~49 days uptime
- Cooldown timer breaks
- **SAFETY HAZARD:** Thermal runaway protection may fail

**Examples of Affected Code:**
1. **Watchdog (Line 1136):** `unsigned long loopElapsedMs = now - watchdogLastLoopMs;`
2. **Thermal Runaway (Line 1191):** `unsigned long onTimeMs = now - maxOnTimeStartMs;`
3. **Button Hold (Line 1397):** `unsigned long heldNow = now - btnPressStartMs;`
4. **Sleep Logic (Line 2065):** `(now - lastActivityMs)`

**Fix:** Use signed time delta calculation or proper overflow handling
```cpp
// Safe time delta function
static inline long timeDelta(unsigned long now, unsigned long before) {
  return (long)(now - before);  // Handles overflow correctly
}

// Usage:
long held = timeDelta(now, btnPressStartMs);
if (held < 0) held = 0;  // Clamp if negative (shouldn't happen in normal operation)
if (held >= BUTTON_LONG_MS) { ... }
```

**Alternative Fix:** Check for overflow explicitly
```cpp
unsigned long loopElapsed;
if (now >= watchdogLastLoopMs) {
  loopElapsed = now - watchdogLastLoopMs;
} else {
  // millis() wrapped; assume stall if wrap exceeds reasonable threshold
  loopElapsed = WATCHDOG_TIMEOUT_MS + 1;  // Force fault
}
```

---

### BUG #3: Divide-by-Zero Risk in PWM Percentage Calculations
**Severity:** 🔴 CRITICAL  
**File:** ESPAtomizer.ino  
**Locations:** Lines 1091, 2099, 2476, 2675, 2973

**Issue:** Multiple divisions by `PWM_MAX` without zero check
```cpp
float drive = (float)pidOutput / (float)PWM_MAX;  // ❌ Line 1091
if (elapsed < (unsigned long)((duty / (double)PWM_MAX) * relayWindowMs)) { // ❌ Line 2099
outPct = (int)((pidOutput / (double)PWM_MAX) * 100.0 + 0.5);  // ❌ Line 2675
```

**Impact:** 
- Division by zero if `PWM_MAX` is misconfigured as 0
- Undefined behavior (crash or incorrect values)
- Could brick device if it happens during critical operation

**Conditions:**
- `PWM_RES_BITS` set to 0 by mistake → `PWM_MAX = (1 << 0) - 1 = 0`
- Manual override of `PWM_MAX` constant

**Fix:** Add compile-time and runtime checks
```cpp
// Compile-time check
#if PWM_MAX == 0
  #error "PWM_MAX cannot be zero! Check PWM_RES_BITS configuration."
#endif

// Runtime safe wrapper
static inline double safePercentage(double value) {
  if (PWM_MAX == 0) return 0.0;
  return (value / (double)PWM_MAX) * 100.0;
}
```

---

### BUG #4: Missing Validation in BLE Write Callbacks
**Severity:** 🔴 CRITICAL (Security + Safety)  
**File:** ESPAtomizer.ino  
**Locations:** Lines 560-630

**Issue:** BLE characteristics validate ranges but don't validate string format before conversion
```cpp
if (data && len > 0) {
  std::string s((char*)data, len);
  double newSp = atof(s.c_str());  // ❌ No validation that string contains valid number!
  if (isnan(newSp) || newSp < (double)ENC_MIN_C || newSp > (double)ENC_MAX_C) {
    Serial.printf("[BLE] WARNING: Rejected invalid setpoint=%.1f\n", newSp);
    return;
  }
  setpointC = newSp;
}
```

**Impact:**
- Malformed BLE data can cause `atof()` to return 0.0 or garbage
- `atof("ABC")` returns 0.0 → sets setpoint to 0°C (potentially dangerous)
- `atof("999999999999")` could cause overflow
- Could be exploited to cause unexpected behavior

**Attack Vectors:**
1. Send non-numeric string → setpoint goes to 0°C
2. Send very large number → potential overflow
3. Send special characters → undefined behavior

**Fix:** Validate string format before conversion
```cpp
static bool isValidNumericString(const char* str) {
  if (!str || *str == '\0') return false;
  char* endptr;
  strtod(str, &endptr);
  return (*endptr == '\0' && endptr != str);  // Entire string was consumed
}

// In callback:
if (data && len > 0) {
  std::string s((char*)data, len);
  if (!isValidNumericString(s.c_str())) {
    Serial.printf("[BLE] ERROR: Invalid numeric format: '%s'\n", s.c_str());
    return;
  }
  double newSp = atof(s.c_str());
  // ... rest of validation
}
```

---

## 🟠 HIGH PRIORITY BUGS (Fix Before Production)

### BUG #5: Blocking delay() Calls in Main Loop
**Severity:** 🟠 HIGH  
**File:** ESPAtomizer.ino  
**Locations:** Lines 769, 771, 799, 879, 890, 906, 909, 1541, 1564, 2079, 2325

**Issue:** 11 instances of `delay()` blocking main loop execution
```cpp
delay(1000); // Give serial time to initialize  // Line 769
delay(2000);  // Boot error display  // Line 879
delay(1000);  // Confirmation message  // Line 1541
```

**Impact:**
- Watchdog may trigger during boot delays
- Temperature not monitored during delays
- BLE disconnects possible
- User input ignored
- **Safety concern:** Heater unmonitored for up to 2 seconds

**Examples:**
- **Boot sequence (769-909):** 5.4 seconds of blocking delays
- **Config saves (1541-1564):** 3 seconds blocked
- **Digital test (2325):** 1 second blocked

**Fix:** Replace with non-blocking state machine or remove unnecessary delays
```cpp
// Instead of:
delay(2000);

// Use:
static unsigned long splashStartMs = 0;
if (splashStartMs == 0) splashStartMs = millis();
if (millis() - splashStartMs < 2000) {
  // Still showing splash
  return;  // Don't proceed
}
// Continue with boot...
```

---

### BUG #6: Charger Removal Ramp Calculation Error
**Severity:** 🟠 HIGH  
**File:** ESPAtomizer.ino  
**Locations:** Lines 1281-1289, 1914-1920

**Issue:** Two different ramp calculations with conflicting logic
```cpp
// Location 1 (checkBatterySafety):
int rampStep = (int)(rampElapsed / 100); // ❌ Steps every 100ms
if (rampStep >= CHARGER_RAMP_DOWN_STEPS) { ... }

// Location 2 (main loop):
int rampStep = (int)((rampElapsed * CHARGER_RAMP_DOWN_STEPS) / (CHARGER_REMOVED_WINDOW_MS * 2)); 
// ❌ Different formula! Uses 10s window (WINDOW_MS * 2)
```

**Impact:**
- Inconsistent ramp-down behavior
- Location 1: 10 steps × 100ms = 1 second ramp
- Location 2: 10 steps over 10 seconds = much slower
- **Safety concern:** Sudden power drop vs. gradual ramp confusion

**Root Cause:** Duplicate code with different implementation

**Fix:** Consolidate into single function with consistent formula
```cpp
static int calculateRampStep(unsigned long elapsedMs) {
  // Ramp down over CHARGER_RAMP_DURATION_MS (e.g., 5000ms)
  if (elapsedMs >= CHARGER_RAMP_DURATION_MS) {
    return CHARGER_RAMP_DOWN_STEPS;
  }
  return (int)((elapsedMs * CHARGER_RAMP_DOWN_STEPS) / CHARGER_RAMP_DURATION_MS);
}
```

---

### BUG #7: Preheat Timing Uses millis() Comparison Without Overflow Protection
**Severity:** 🟠 HIGH  
**File:** ESPAtomizer.ino  
**Location:** Line 1956

**Issue:** Preheat end time calculated without overflow handling
```cpp
preheatEndMs = millis() + (unsigned long)rtcU1PreheatMs;  // ❌ Overflow issue
// Later:
if (millis() < preheatEndMs) {  // ❌ Breaks on millis() overflow
  applyOutput((double)PWM_MAX);
}
```

**Impact:**
- If `millis()` overflows during preheat, comparison fails
- Preheat may end immediately or never end
- **Safety issue:** Full power heater may stay on indefinitely

**Example Scenario:**
```
millis() = 4,294,967,000 (near overflow)
preheatEndMs = 4,294,967,000 + 3,000 = 4,294,970,000 (wraps to ~2,704)
millis() = 100 (after overflow)
100 < 2,704 = TRUE ✅ (works by accident)
BUT: if preheat duration > 1000ms, breaks completely
```

**Fix:** Use elapsed time instead of absolute time
```cpp
preheatStartMs = millis();
preheatDurationMs = rtcU1PreheatMs;

// Later:
if ((millis() - preheatStartMs) < preheatDurationMs) {
  applyOutput((double)PWM_MAX);
}
```

---

### BUG #8: Sensor Fault Recovery Never Triggered
**Severity:** 🟠 HIGH  
**File:** ESPAtomizer.ino  
**Location:** Lines 388-395, updateSensorFaultState implementation (not shown in excerpt)

**Issue:** Sensor recovery threshold defined but recovery logic may not be working as intended

**Analysis Needed:** Need to see `updateSensorFaultState()` implementation to verify:
1. Is `sensorValidCount` incremented correctly?
2. Is `SENSOR_VALID_RECOVERY` threshold (3 reads) checked?
3. Does recovery clear `sensorFaulted` flag?

**Potential Impact:**
- Once sensor faults, may never recover even if readings become valid
- User must power cycle to clear fault
- **Safety concern:** Device may stay disabled unnecessarily

**Recommendation:** Review `updateSensorFaultState()` and add explicit recovery logging

---

### BUG #9: Thermal Runaway Auto-Recovery Condition Error
**Severity:** 🟠 HIGH  
**File:** ESPAtomizer.ino  
**Location:** Lines 1208-1213

**Issue:** Recovery checks temperature and output, but doesn't verify runaway cause is resolved
```cpp
if (thermalRunawayFaulted && pidOutput < (MAX_PWM_THRESHOLD * 0.5f)) {
  if (!isnan(inputC) && inputC < (setpointC + (THERMAL_RUNAWAY_MARGIN_C * 0.5f))) {
    thermalRunawayFaulted = false;  // ❌ Cleared without verifying cause
    Serial.println("[THERMAL] Fault cleared; output normalized and temp stable");
  }
}
```

**Impact:**
- Runaway may clear prematurely
- Root cause (hardware fault, PID divergence) not addressed
- Could immediately re-fault

**Fix:** Add hysteresis and verify stability
```cpp
static unsigned long thermalRecoveryStartMs = 0;
#define THERMAL_RECOVERY_DURATION_MS 10000  // 10 seconds stable required

if (thermalRunawayFaulted) {
  if (pidOutput < (MAX_PWM_THRESHOLD * 0.5f) && 
      !isnan(inputC) && inputC < (setpointC + (THERMAL_RUNAWAY_MARGIN_C * 0.5f))) {
    // Conditions met for recovery
    if (thermalRecoveryStartMs == 0) {
      thermalRecoveryStartMs = millis();
    } else if ((millis() - thermalRecoveryStartMs) >= THERMAL_RECOVERY_DURATION_MS) {
      // Stable for 10 seconds
      thermalRunawayFaulted = false;
      thermalRecoveryStartMs = 0;
      Serial.println("[THERMAL] Fault cleared after sustained stability");
    }
  } else {
    // Conditions not met; reset timer
    thermalRecoveryStartMs = 0;
  }
}
```

---

### BUG #10: Encoder Sanity Check Resets Counters Silently
**Severity:** 🟠 HIGH  
**File:** ESPAtomizer.ino  
**Location:** Lines 1736-1741

**Issue:** Large encoder delta assumed to be noise and silently zeroed
```cpp
const int32_t SANITY_MAX = 200;
if (abs(delta) > SANITY_MAX) {
  Serial.printf("[ENC] Large delta=%ld -> zeroing counters for safety\n", (long)delta);
  encoderZeroCounters();
  // skip processing this iteration  // ❌ Data loss, no recovery
}
```

**Impact:**
- Legitimate rapid encoder turns are discarded
- User expects setpoint change but nothing happens
- No feedback that input was ignored (only serial message)
- **UX issue:** Confusing and frustrating

**Scenarios:**
- User rapidly spins encoder 50 clicks (200 edges) → all ignored
- Encoder noise/bounce generates large delta → resets position

**Fix:** Cap delta instead of resetting
```cpp
const int32_t SANITY_MAX = 200;
if (abs(delta) > SANITY_MAX) {
  Serial.printf("[ENC] Large delta=%ld -> capping to %ld for safety\n", 
                (long)delta, (long)(SANITY_MAX * (delta > 0 ? 1 : -1)));
  delta = SANITY_MAX * (delta > 0 ? 1 : -1);  // Cap to max, preserve direction
}
```

---

### BUG #11: Battery Percentage Calculation Doesn't Handle Charging State
**Severity:** 🟠 HIGH  
**File:** battery.h  
**Location:** Lines 44-47

**Issue:** Linear interpolation between MIN and MAX voltage doesn't account for battery chemistry charging curve
```cpp
double p = (batteryVoltage - BAT_MIN_V) / (BAT_MAX_V - BAT_MIN_V) * 100.0;
if (p < 0) p = 0; if (p > 100) p = 100;
batteryPercent = (int)(p + 0.5);
```

**Impact:**
- Inaccurate percentage (could show 80% when actually 60%)
- Battery life estimation unreliable
- User may be surprised by sudden shutdowns

**Example:**
- LiPo battery: 3.7V = 50% (not 50% by voltage range)
- Voltage drops non-linearly during discharge

**Fix:** Use lookup table or polynomial approximation for LiPo curve
```cpp
// Simplified LiPo discharge curve (5-point lookup)
static int voltageToPercent(double v) {
  if (v >= 4.20) return 100;
  if (v >= 4.00) return 90 + (int)((v - 4.00) * 50);  // 4.0-4.2V = 90-100%
  if (v >= 3.80) return 60 + (int)((v - 3.80) * 150); // 3.8-4.0V = 60-90%
  if (v >= 3.60) return 30 + (int)((v - 3.60) * 150); // 3.6-3.8V = 30-60%
  if (v >= 3.40) return 10 + (int)((v - 3.40) * 100); // 3.4-3.6V = 10-30%
  if (v >= 3.20) return (int)((v - 3.20) * 50);       // 3.2-3.4V = 0-10%
  return 0;
}
```

---

### BUG #12: Confirmation Dialog Timeout Check Missing Millis Overflow Handling
**Severity:** 🟠 HIGH  
**File:** ESPAtomizer.ino  
**Location:** Lines 1300-1308

**Issue:** Timeout comparison uses unsigned subtraction
```cpp
if ((now - confirmationStartMs) > ENC_CONFIRM_TIMEOUT_MS) {  // ❌ Underflow risk
  Serial.printf("[ENC_CONFIRM] Timeout: Change from %.1fC to %.1fC cancelled\n", 
                setpointC, pendingSetpointC);
  awaitingConfirmation = false;
  return;
}
```

**Impact:**
- After millis() overflow, timeout may never trigger
- User stuck awaiting confirmation indefinitely
- Must restart device to clear state

**Fix:** Same as BUG #2 - use signed arithmetic
```cpp
long elapsed = timeDelta(now, confirmationStartMs);
if (elapsed < 0) elapsed = 0;  // Shouldn't happen normally
if (elapsed > ENC_CONFIRM_TIMEOUT_MS) { ... }
```

---

## 🟡 MEDIUM PRIORITY ISSUES

### ISSUE #13: Continuous Runtime Limit Calculation Doesn't Reset on Disable
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1330-1346

**Code:**
```cpp
if (systemEnabled && pidOutput > 0) {
  if (continuousRunStartMs == 0) {
    continuousRunStartMs = now;
  } else if (now - continuousRunStartMs >= MAX_CONTINUOUS_RUN_MS) {
    // ... disable system
  }
} else {
  continuousRunStartMs = 0;  // Reset when disabled or output is zero
}
```

**Issue:** Reset happens when `systemEnabled == false` OR `pidOutput == 0`, but:
- If user manually sets output to 0 briefly, timer resets
- Defeats the purpose of tracking continuous runtime
- Should only reset when system explicitly disabled

**Fix:**
```cpp
if (systemEnabled && pidOutput > 0) {
  if (continuousRunStartMs == 0) {
    continuousRunStartMs = now;
  } else if (now - continuousRunStartMs >= MAX_CONTINUOUS_RUN_MS) {
    // ... disable
  }
} else if (!systemEnabled) {
  continuousRunStartMs = 0;  // Only reset when explicitly disabled
}
```

---

### ISSUE #14: PWM Ramp Rate Limiting Uses Last Applied Value From Different Context
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1217-1230

**Issue:** `lastAppliedOutput` is static variable shared across function calls, but function can be called from different contexts (PID vs manual vs preheat)

```cpp
static double applyOutputWithRampLimit(double newOutput) {
  double maxDelta = MAX_PWM_RAMP_RATE;
  double delta = newOutput - lastAppliedOutput;  // ❌ Shared state
  
  if (delta > maxDelta) {
    newOutput = lastAppliedOutput + maxDelta;
  } else if (delta < -maxDelta) {
    newOutput = lastAppliedOutput - maxDelta;
  }
  
  lastAppliedOutput = newOutput;  // ❌ Updates shared state
  return newOutput;
}
```

**Impact:**
- Ramp rate works correctly for gradual PID changes
- Sudden context switch (e.g., PID → preheat → PID) causes unexpected ramp
- Example: PID at 50% → preheat 100% → PID back to 50% = ramped down slowly (incorrect)

**Fix:** Pass context or separate ramp state
```cpp
static double applyOutputWithRampLimit(double newOutput, bool forceClear = false) {
  if (forceClear) {
    lastAppliedOutput = newOutput;
    return newOutput;
  }
  // ... rest of function
}

// When switching modes:
applyOutputWithRampLimit(PWM_MAX, true);  // Preheat without ramp
```

---

### ISSUE #15: Deep Sleep Wake Pin Check Happens Too Late
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 2068-2083

**Issue:** Wake pins checked after deciding to sleep, not before
```cpp
if ((now - lastActivityMs) >= SLEEP_ON_IDLE_MS && !menuActive) {
  Serial.printf("[SLEEP] idle %lums >= %lums, preparing deep sleep\n", ...);
  
  // Check wake pins (encoder A/B/SW) before sleeping
  // ... pin checks ...
  
  if (anyPinLow) {
    Serial.println(F("[SLEEP] wake pin active, aborting deep sleep"));
    return;  // ❌ Already printed "preparing" message
  }
```

**Impact:**
- Confusing serial output: "preparing deep sleep" followed immediately by "aborting"
- Log analysis difficult
- Minor UX issue

**Fix:** Check pins before printing message
```cpp
if ((now - lastActivityMs) >= SLEEP_ON_IDLE_MS && !menuActive) {
  // Check wake pins FIRST
  int a = digitalRead(ENC_PIN_A);
  int b = digitalRead(ENC_PIN_B);
  int sw = digitalRead(ENC_PIN_SW);
  bool anyPinLow = (a == LOW || b == LOW || sw == LOW);
  
  if (anyPinLow) {
    // Silently skip sleep without logging
    return;
  }
  
  // Now log and proceed
  Serial.printf("[SLEEP] idle %lums >= %lums, entering deep sleep\n", ...);
  // ... sleep preparation
}
```

---

### ISSUE #16: NVS Preferences Not Checked For Success Before Use
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Locations:** Lines 784-793, 1423-1570

**Issue:** `prefs.begin()` and `prefs.getString()` not checked for failures
```cpp
Preferences prefs;
prefs.begin("espatom", true);  // ❌ No check if this succeeded
String v;
v = prefs.getString("def_sp", ""); // ❌ No check if valid
if (v.length()) rtcDefaultSetpoint = atof(v.c_str());
```

**Impact:**
- If NVS corrupted or full, reads fail silently
- May load garbage values
- `atof("")` returns 0.0 → incorrect defaults

**Fix:** Check return values
```cpp
Preferences prefs;
if (!prefs.begin("espatom", true)) {
  Serial.println(F("[PREF] Failed to open NVS namespace"));
  return;  // Use defaults
}
String v = prefs.getString("def_sp", "");
if (v.length() > 0 && isValidNumericString(v.c_str())) {
  rtcDefaultSetpoint = atof(v.c_str());
}
prefs.end();
```

---

### ISSUE #17: Thermal Runaway Check Uses Same Variable For Two Different Conditions
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1165-1205

**Issue:** `thermalRunawayFaulted` set by three different conditions but doesn't track which one triggered

**Conditions:**
1. Temperature exceeds absolute max (750°C)
2. Temperature exceeds setpoint + margin (50°C)
3. High output (>90%) for extended time without temp rise

**Impact:**
- Recovery logic doesn't know which condition caused fault
- May clear fault inappropriately
- Diagnostics unclear about root cause

**Fix:** Add fault type enum
```cpp
enum ThermalFaultType {
  THERMAL_FAULT_NONE = 0,
  THERMAL_FAULT_ABSOLUTE_MAX = 1,
  THERMAL_FAULT_MARGIN_EXCEEDED = 2,
  THERMAL_FAULT_STALL = 3
};
static ThermalFaultType thermalFaultType = THERMAL_FAULT_NONE;

// When faulting:
if (!isnan(inputC) && inputC > THERMAL_RUNAWAY_TEMP_C) {
  if (!thermalRunawayFaulted) {
    thermalRunawayFaulted = true;
    thermalFaultType = THERMAL_FAULT_ABSOLUTE_MAX;
    // ... log specific fault
  }
}
```

---

### ISSUE #18: Watchdog Timeout Logic Doesn't Account For Boot Time
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1136-1159

**Issue:** Watchdog checks elapsed time since last loop, but during first loop iteration `watchdogLastLoopMs` is 0
```cpp
unsigned long loopElapsedMs = now - watchdogLastLoopMs;  // ❌ Very large on first loop!
if (loopElapsedMs > WATCHDOG_TIMEOUT_MS) {
  if (!watchdogFaulted) {
    watchdogFaulted = true;
    logError("WATCHDOG", "Main loop stall detected");
  }
}
```

**Impact:**
- First loop iteration has huge elapsed time (millis() - 0)
- May trigger watchdog fault immediately on boot
- Likely mitigated by `if (!watchdogFaulted)` guard, but still incorrect logic

**Fix:** Initialize watchdog timestamp in setup()
```cpp
void setup() {
  // ... other setup
  watchdogLastLoopMs = millis();  // Initialize before first loop
  pidLastComputeMs = millis();
}
```

---

### ISSUE #19: BLE Notification Throttling Not Implemented
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1836-1844

**Issue:** Temperature BLE notification sent every time sensor is read (every 100ms minimum)
```cpp
if (chTemp && bleIsConnected()) {
  char tbuf[32]; snprintf(tbuf, sizeof(tbuf), "%.2f", inputC);
  std::string tv(tbuf);
  chTemp->setValue(tv);
  chTemp->notify();  // ❌ No throttling - 10 notifications/second!
}
```

**Impact:**
- BLE stack overwhelmed with notifications
- Battery drain (radio constantly transmitting)
- iOS app receives unnecessary updates
- May cause connection instability

**Fix:** Add notification throttle
```cpp
#define TEMP_NOTIFY_INTERVAL_MS 1000  // 1 Hz is sufficient
static unsigned long lastTempNotifyMs = 0;

if (chTemp && bleIsConnected()) {
  if ((now - lastTempNotifyMs) >= TEMP_NOTIFY_INTERVAL_MS) {
    lastTempNotifyMs = now;
    char tbuf[32]; snprintf(tbuf, sizeof(tbuf), "%.2f", inputC);
    std::string tv(tbuf);
    chTemp->setValue(tv);
    chTemp->notify();
  }
}
```

---

### ISSUE #20: Battery ADC Sampling Delay in Loop Context
**Severity:** 🟡 MEDIUM  
**File:** battery.h  
**Location:** Lines 40-42

**Issue:** `sampleBattery()` contains multiple 1ms delays in main loop context
```cpp
for (int i = 0; i < BAT_SAMPLES; ++i) { 
  acc += analogRead(BAT_PIN); 
  delay(1);  // ❌ BAT_SAMPLES * 1ms = blocking in main loop!
}
```

**Impact:**
- If `BAT_SAMPLES = 10`, delays 10ms every battery sample
- Blocks watchdog, PID, encoder, display updates
- Adds up over time (called frequently)

**Fix:** Use non-blocking sampling or reduce samples
```cpp
// Option 1: Reduce samples (ADC is stable enough)
#define BAT_SAMPLES 3  // Down from 10

// Option 2: Non-blocking state machine (complex)
static int batSampleIndex = 0;
static uint32_t batAccumulator = 0;
static unsigned long lastBatSampleMs = 0;

void sampleBatteryNonBlocking() {
  if (batSampleIndex < BAT_SAMPLES) {
    if ((millis() - lastBatSampleMs) >= 1) {
      batAccumulator += analogRead(BAT_PIN);
      batSampleIndex++;
      lastBatSampleMs = millis();
    }
  } else {
    // All samples collected, compute
    uint32_t batteryRaw = batAccumulator / BAT_SAMPLES;
    // ... rest of calculation
    batSampleIndex = 0;
    batAccumulator = 0;
  }
}
```

---

### ISSUE #21: Config Mode Editing State Not Cleared on Menu Exit
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1399-1410

**Issue:** `configEditing` flag not reset when exiting config mode
```cpp
if (menuActive) {
  menuIndex = pidMode;
  Serial.println(F("[MENU] Opened"));
} else {
  Serial.println(F("[MENU] Closed"));
  // ❌ configEditing not cleared!
}
```

**Impact:**
- If user opens config, starts editing, then long-presses to exit, `configEditing` stays true
- Next time config opens, encoder immediately edits value without button press
- Confusing UX

**Fix:** Clear all config state on menu close
```cpp
} else {
  Serial.println(F("[MENU] Closed"));
  configMode = false;
  configEditing = false;  // Clear editing state
}
```

---

### ISSUE #22: OLED Display Error Recovery Doesn't Re-Initialize
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 2002-2013

**Issue:** Display update failures logged but no recovery attempted
```cpp
display.display();
unsigned long displayTime = millis() - beforeDisplay;
if (displayTime > 100) {
  Serial.printf("[OLED_PERF] display.display() took %lums (watchdog threshold: %lums)\n", 
                displayTime, WATCHDOG_TIMEOUT_MS);
  // ❌ No recovery action taken
}
```

**Impact:**
- If display fails (I2C timeout, hardware issue), just logs warning
- Display stays broken until reboot
- User loses visual feedback

**Fix:** Attempt re-initialization on repeated failures
```cpp
static int displayFailureCount = 0;
if (displayTime > 100) {
  displayFailureCount++;
  if (displayFailureCount > 5) {
    Serial.println(F("[OLED] Multiple slow updates, attempting re-init"));
    displayAvailable = tryInitOLED(OLED_SDA, OLED_SCL);
    displayFailureCount = 0;
  }
} else {
  displayFailureCount = 0;  // Reset on success
}
```

---

### ISSUE #23: Manual Mode Hold-to-Heat Doesn't Check System Enabled
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1723-1727

**Issue:** Hold-to-heat activates even if system is disabled
```cpp
if (systemEnabled && manualMode && btnStablePressed) {
  unsigned long held = now - btnPressStartMs;
  if (held >= (BUTTON_LONG_MS + 500) && !btnHoldHeating) { 
    btnHoldHeating = true; 
    pidOutputBeforeHold = pidOutput; 
    pidOutput = PWM_MAX;  // ❌ Sets PWM_MAX unconditionally
    Serial.println(F("[Manual] Hold heating ON (100%)"));
  }
}
```

**Impact:**
- Checks `systemEnabled` in condition, but then output is applied in main loop regardless
- If safety faults (battery low, thermal runaway) active, this bypasses them
- **Safety concern:** Can force heater on during fault condition

**Fix:** Add safety check in output application
```cpp
if (btnHoldHeating && !systemEnabled) {
  // Safety override: clear hold-heat if system disabled
  btnHoldHeating = false;
  pidOutput = pidOutputBeforeHold;
}
```

---

### ISSUE #24: Encoder Rate Limiting Doesn't Account For Menu Navigation
**Severity:** 🟡 MEDIUM  
**File:** ESPAtomizer.ino  
**Location:** Lines 1752-1754

**Issue:** Same rate limit (50ms) applied to menu navigation and value adjustment
```cpp
if ((now - lastEncoderInputMs) < ENC_RATE_LIMIT_MS) {
  // Rate limited: skip this step  // ❌ Applies to menu too!
}
```

**Impact:**
- Menu navigation feels sluggish (50ms between items)
- Value adjustment correctly rate-limited
- UX inconsistency

**Fix:** Separate rate limits for different contexts
```cpp
#define ENC_RATE_LIMIT_VALUE_MS 50   // For setpoint/manual adjustments
#define ENC_RATE_LIMIT_MENU_MS 20    // Faster for menu navigation

unsigned long rateLimit = (menuActive || configMode) ? 
                           ENC_RATE_LIMIT_MENU_MS : ENC_RATE_LIMIT_VALUE_MS;
if ((now - lastEncoderInputMs) < rateLimit) {
  // Rate limited
}
```

---

## 🟢 LOW PRIORITY / IMPROVEMENTS

### IMPROVEMENT #25: Magic Number for Charging Detection (Line 1370)
**Current:** `charging = (batteryPercent < 100 && batteryVoltage > BAT_MIN_V);`  
**Issue:** Hardcoded 100% threshold  
**Fix:** `#define BATTERY_FULL_PERCENT 100` and use constant

---

### IMPROVEMENT #26: Serial Streaming Toggle Command Uses 'Z'
**File:** Line 2331  
**Issue:** Non-intuitive command letter  
**Fix:** Change to 'S' for "Stream" or 'V' for "Verbose"

---

### IMPROVEMENT #27: Preheat Duration Not Validated
**File:** Lines 2414-2423  
**Issue:** User can set preheat to 0ms or very large values  
**Fix:** Add validation `if (v > 0 && v < 60000) rtcU1PreheatMs = v;`

---

### IMPROVEMENT #28: BLE Advertising Interval Not Applied Until Restart
**File:** Lines 1498-1507  
**Issue:** User sets interval but must manually restart advertising  
**Fix:** Restart advertising automatically after change

---

### IMPROVEMENT #29: Absolute Temperature Limits Not Configurable
**File:** Lines 211-213  
**Issue:** 350°C max hardcoded  
**Improvement:** Make `#define` with compile-time option

---

### IMPROVEMENT #30: Boot Self-Test Results Not Accessible Via BLE
**Issue:** Only printable via serial  
**Improvement:** Add BLE characteristic for boot test status

---

### IMPROVEMENT #31: Error Log lastError String Can Overflow
**File:** Lines 232-233  
**Issue:** `snprintf` with 64-char buffer but no length check on input  
**Fix:** Always use `snprintf(..., sizeof(buffer), ...)` with correct size

---

### IMPROVEMENT #32: Relay Mode Window Timing Uses Modulo With Possible Edge Case
**File:** Line 2097  
**Code:** `unsigned long elapsed = (now - windowStartTime) % relayWindowMs;`  
**Issue:** If `relayWindowMs == 0`, division by zero  
**Fix:** Add check `if (relayWindowMs == 0) return;` at function start

---

### IMPROVEMENT #33: Deep Sleep Wake Source Not Logged
**Issue:** User doesn't know which pin caused wake  
**Fix:** Add `esp_sleep_get_wakeup_cause()` logging on boot after deep sleep

---

### IMPROVEMENT #34: NAN Temperature Printing Could Be More Descriptive
**File:** Multiple locations  
**Issue:** `Serial.printf("temp=%.2fC", inputC)` prints "-nan" or "nan"  
**Fix:** Check and print "SENSOR_FAULT" or "NO_READING" instead

---

### IMPROVEMENT #35: Config Save Doesn't Provide Failure Feedback To User
**File:** Lines 1441-1448  
**Issue:** `prefs.putString()` return value ignored  
**Fix:** Check return and display "Save Failed" on OLED if error

---

### IMPROVEMENT #36: Factory Reset Confirmation Not Required
**File:** Config menu "Factory Reset" item  
**Issue:** One button press resets everything  
**Risk:** Accidental activation  
**Fix:** Require long-press or double-tap confirmation

---

### IMPROVEMENT #37: PID Tuning Values Not Bounds-Checked On Restore
**File:** RTC memory restore (lines 226-237)  
**Issue:** Restored Kp/Ki/Kd values not validated  
**Risk:** Corrupted RTC memory could cause unstable PID  
**Fix:** Validate restored values before applying

---

### IMPROVEMENT #38: Continuous Runtime Timer Should Accumulate, Not Reset
**Current:** Timer resets when output goes to 0  
**Issue:** User can bypass 30-min limit by briefly disabling  
**Fix:** Accumulate runtime and require longer cooldown

---

### IMPROVEMENT #39: Watchdog Fault Doesn't Log Stack Trace Or Core Dump
**Issue:** When watchdog triggers, limited debug info  
**Improvement:** Use ESP32 core dump feature for post-mortem analysis

---

## Summary By Category

### Safety-Critical Bugs
1. ✅ BUG #2: Unsigned overflow in safety timers (watchdog, thermal)
2. ✅ BUG #3: Divide-by-zero in PWM calculations
3. ✅ BUG #4: BLE write validation missing
4. ✅ BUG #7: Preheat timing overflow
5. ✅ BUG #9: Thermal runaway recovery too aggressive
6. ✅ ISSUE #23: Hold-to-heat bypasses safety

### Logic Errors
1. ✅ BUG #1: Battery warning condition impossible
2. ✅ BUG #6: Charger ramp calculation mismatch
3. ✅ BUG #8: Sensor recovery may not work
4. ✅ ISSUE #13: Runtime limit reset logic
5. ✅ ISSUE #17: Thermal fault type not tracked
6. ✅ ISSUE #21: Config editing state not cleared

### Performance Issues
1. ✅ BUG #5: 11 blocking delay() calls
2. ✅ ISSUE #19: BLE notification spam (10/sec)
3. ✅ ISSUE #20: Battery sampling delays in loop
4. ✅ ISSUE #22: Display error no recovery

### UX/Usability
1. ✅ BUG #10: Encoder sanity check too aggressive
2. ✅ ISSUE #15: Sleep wake pin check too late
3. ✅ ISSUE #24: Menu navigation sluggish
4. ✅ IMPROVEMENT #36: Factory reset too easy

### Data Accuracy
1. ✅ BUG #11: Battery percentage linear (should be non-linear)
2. ✅ IMPROVEMENT #34: NAN printing unclear
3. ✅ IMPROVEMENT #37: PID values not validated on restore

---

## Recommended Fix Priority

### Phase 1: Immediate (Ship-Blocker)
1. BUG #2: Fix unsigned overflow in all time calculations
2. BUG #3: Add PWM_MAX divide-by-zero protection
3. BUG #4: Validate BLE write string format
4. BUG #1: Fix battery warning logic

### Phase 2: Pre-Production (Safety)
1. BUG #7: Fix preheat timing
2. BUG #9: Improve thermal recovery
3. ISSUE #23: Fix hold-to-heat safety bypass
4. BUG #5: Remove blocking delays
5. BUG #6: Fix charger ramp calculation

### Phase 3: Quality Improvement
1. ISSUE #19: Throttle BLE notifications
2. ISSUE #20: Non-blocking battery sampling
3. BUG #11: Better battery percentage
4. ISSUE #24: Faster menu navigation
5. BUG #10: Better encoder sanity handling

### Phase 4: Polish (Optional)
- All remaining improvements (#25-#39)

---

## Testing Recommendations

### Critical Test Cases
1. **Millis Overflow:** Run device for 49+ days or simulate overflow
2. **BLE Malicious Input:** Send invalid strings to all characteristics
3. **Thermal Runaway:** Force high temp/output and verify shutdown
4. **Battery Dropout:** Remove battery during operation
5. **Encoder Stress:** Rapid spinning, bouncing, noise injection

### Safety Validation
1. Watchdog triggers when loop stalls
2. Thermal limits enforced under all conditions
3. Battery cutoff works reliably
4. Sensor fault disables heater
5. All safety checks work after millis() overflow

---

**Report Generated:** December 30, 2025  
**Recommended Review Cadence:** Every major firmware update  
**Next Review:** After Phase 1 fixes implemented
