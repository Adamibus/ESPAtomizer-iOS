# ESPAtomizer Refactoring - Phase 1 Analysis

**File**: ESPAtomizer.ino  
**Total Lines**: 2,858  
**Status**: Analysis Complete  
**Date**: 2026-05-27

---

## 1. Global Variables Inventory (50+ identified)

### 1A. BLE Globals (20 variables)
```cpp
NimBLEServer* bleServer = nullptr;
NimBLECharacteristic *chEnable, *chSetpoint, *chKp, *chKi, *chKd, *chMode, *chTemp, 
                     *chOut, *chBat, *chModeRead, *chDefaultSp, *chUnit, *chPreU1Enabled, 
                     *chPreU1Ms, *chPreU2Enabled, *chPreU2Ms, *chTcStatus; // 17 characteristics
char lastBleEvent[32] = "none";
```
**Issue**: All BLE globals at sketch level; should move to ble_manager

### 1B. OLED Globals (3 variables)
```cpp
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool displayAvailable = false;
int oledFailureCount = 0;
unsigned long lastOledSuccess = 0;
```

### 1C. Battery Globals (10 variables)
```cpp
double batteryVoltage = NAN;
int batteryPercent = -1;
bool batteryLow = false;
static double batteryVoltagePrevious = 0.0f;
static unsigned long batteryVoltageCheckMs = 0;
static bool chargerRemovedFault = false;
static bool batteryLowWarning = false;
static bool batteryHysteresisActive = false;
static unsigned long chargerRampStartMs = 0;
static bool chargerRampActive = false;
```
**Issue**: Battery state scattered; should consolidate into `BatteryState` struct

### 1D. Boot Self-Test Flags (5 variables)
```cpp
static bool bootTestSensorOk = true;
static bool bootTestHeaterOk = true;
static bool bootTestEncoderOk = true;
static bool bootTestButtonOk = true;
static bool bootTestBatteryOk = true;
```

### 1E. PID State (7 variables)
```cpp
static double Kp = 10.0, Ki = 0.5, Kd = 50.0;
static double setpointC = 200.0;
static double inputC = 0.0;
static double pidOutput = 0.0;
static PID tempPID(&inputC, &pidOutput, &setpointC, Kp, Ki, Kd, DIRECT);

// RTC Memory for persistence (18 RTC variables)
RTC_DATA_ATTR static double rtcSetpointC, rtcKp, rtcKi, rtcKd, rtcManualMode, rtcSystemEnabled, rtcPidMode, rtcPidOutput;
RTC_DATA_ATTR static double rtcU1Kp, rtcU1Ki, rtcU1Kd, rtcU1Sp;
RTC_DATA_ATTR static double rtcU2Kp, rtcU2Ki, rtcU2Kd, rtcU2Sp;
RTC_DATA_ATTR static double rtcDefaultSetpoint;
RTC_DATA_ATTR static bool rtcBleEnabled, rtcBleOledIndicator, rtcTempUnitIsC;
RTC_DATA_ATTR static int rtcBleNameIndex;
RTC_DATA_ATTR static unsigned long rtcBleAdvIntervalMs;
RTC_DATA_ATTR static bool rtcU1PreheatEnabled, rtcU2PreheatEnabled;
RTC_DATA_ATTR static unsigned long rtcU1PreheatMs, rtcU2PreheatMs;
RTC_DATA_ATTR static char rtcLastBootError[64];
```

### 1F. System Health & Faults (10+ variables)
```cpp
SystemHealth systemHealth;  // struct with 8 fields
ErrorLog errorLog;          // struct with 6 fields + strings

// Thermal Runaway
static unsigned long maxOnTimeStartMs = 0;
static float lastAppliedOutput = 0.0f;
static bool thermalRunawayFaulted = false;
static unsigned long thermalRunawayMs = 0;

// Sensor Faults
static int sensorFaultCount = 0;
static int sensorValidCount = 0;
static bool sensorFaulted = false;
static unsigned long sensorFaultMs = 0;

// Watchdog
static unsigned long watchdogLastLoopMs = 0;
static unsigned long pidLastComputeMs = 0;
static bool watchdogFaulted = false;
```

### 1G. Mode & Menu State (10 variables)
```cpp
static bool manualMode = false;
static bool systemEnabled = (INIT_POWER_ON != 0);
static bool remoteControlEnabled = true;
static bool menuActive = false;
static int menuIndex = 0;
static int pidMode = 0;  // 0=AUTO, 1=MAN, 2=U1, 3=U2, 4=Config
static bool configMode = false;
static int configIndex = 0;
static bool configEditing = false;

// Encoder input protection
static unsigned long lastEncoderInputMs = 0;
static double pendingSetpointC = 0.0f;
static bool awaitingConfirmation = false;
static unsigned long confirmationStartMs = 0;
```

### 1H. Button State (6 variables)
```cpp
static bool btnStable = true;
static bool btnLastRead = false;
static unsigned long btnLastChangeMs = 0;
static bool btnStablePressed = false;
static unsigned long btnPressStartMs = 0;
static bool btnLongHandled = false;

// Manual mode hold-to-heat
static bool btnHoldHeating = false;
static double pidOutputBeforeHold = 0;

// Preheat state
static bool preheatActive = false;
static unsigned long preheatEndMs = 0;
static double pidOutputBeforePreheat = 0;
static int preheatWhich = 0;
```

### 1I. LED/Animation State (3 variables)
```cpp
static unsigned long bleAnimExpireMs = 0;
static bool bleAnimIsConnect = false;
char lastEncoderInfo[64] = "enc:none";
```

### 1J. Continuous Run & Limits (3 variables)
```cpp
static unsigned long continuousRunStartMs = 0;
static bool cooldownRequired = false;
static unsigned long cooldownStartMs = 0;
```

### 1K. Timing & Sampling (7 variables)
```cpp
static unsigned long loopIntervalMs = 1000;
static unsigned long lastLoopMs = 0;
static unsigned long lastTempSampleMs = 0;
static unsigned long lastTempPrintMs = 0;
static unsigned long lastActivityMs = 0;
static unsigned long lastDebugMs = 0;
static unsigned long lastSleepPrintMs = 0;
static unsigned long lastApplyPrintMs = 0;
static int lastAppliedDuty = -1;
```

### 1L. Diagnostics & Counters (6 variables)
```cpp
static unsigned long loopIterations = 0;
static unsigned long pidComputations = 0;
static unsigned long outputChanges = 0;
static unsigned long bleWrites = 0;
static unsigned long sensorReads = 0;
static unsigned long sensorReadsFailed = 0;
```

### 1M. Feature Flags & Config (4 variables)
```cpp
static bool serialStreamingEnabled = true;
static bool disableDeepSleepRuntime = false;
static unsigned long windowStartTime = 0;  // for relay mode
```

### 1N. BLE Config (5 variables)
```cpp
static unsigned long bleAnimExpireMs = 0;
static bool bleAnimIsConnect = false;
// Preset names
static const char* BLE_NAME_PRESETS[] = { "Adamizer", "ESPAtom", "Atomizer" };
static const int BLE_NAME_PRESET_COUNT = 3;
```

---

## 2. Major Functions (Line Ranges)

| Function | Lines | Purpose | Dependencies |
|----------|-------|---------|--------------|
| `timeDelta()` | 113-115 | Safe time math | None |
| `isValidNumericString()` | 118-126 | Input validation | None |
| `pwmInit()` | 559-583 | PWM initialization | OUTPUT_PIN, PWM_* macros |
| `pwmWrite(int)` | 585-595 | PWM write | OUTPUT_PIN, PWM_* macros |
| `setupBLE()` | 897-1077 | BLE service setup | ble.h, global BLE pointers |
| **`ChCallbacks::onWrite()`** | 778-895 | **BLE write handler (MASSIVE)** | **Preferences, all RTC vars, applyOutput()** |
| `setup()` | 1080-1400+ | Sketch initialization | ALL subsystems |
| `loop()` | 1600-1900+ | **Main loop (MASSIVE)** | **ALL** |
| `applyOutput()` | 2100-2150 | Apply PWM duty | tempPID, rtcPidOutput, Preferences |
| `applyRelayWindow()` | 2080-2110 | Relay emulation | windowStartTime |
| `applyPidMode()` | 2150-2250 | Switch PID modes | rtcU1*, rtcU2*, tempPID, applyOutput |
| `applyOutputWithRampLimit()` | 2030-2080 | Ramp PWM changes | lastAppliedOutput, MAX_PWM_RAMP_RATE |
| `checkWatchdog()` | 1950-2000 | Watchdog timeout check | watchdogLastLoopMs, pidLastComputeMs |
| `checkThermalRunaway()` | 2000-2050 | Thermal protection | inputC, setpointC, maxOnTimeStartMs |
| `checkBatterySafety()` | 1850-1950 | Battery monitoring | batteryVoltage, chargerRampActive |
| `updateSensorFaultState()` | 1750-1850 | Sensor fault detection | sensorFaultCount, sensorValidCount, inputC |
| `handleSerial()` | 2250-2700+ | **Serial CLI (MASSIVE, 15+ commands)** | **Most globals** |
| `updateDisplay()` | 2700-2850+ | OLED rendering | Menu state, PID state, battery, temp |
| `handleConfirmationDialog()` | 2000-2050 | User confirmation | awaitingConfirmation, pendingSetpointC |
| `isExtremeSetpointChange()` | 1950-2000 | Validate SP change | setpointC, ENC_EXTREME_DELTA_C |
| `runBootSelfTests()` | 2850-2858+ | Hardware self-tests | bootTest* flags |
| `logError()` | 2800-2820 | Error logging | errorLog |
| `checkSystemHealth()` | 2820-2850 | Health check | systemHealth |
| `printDiagnostics()` | 2850-2858+ | Debug output | All counters & diagnostics |
| `resetErrorCounters()` | Similar | Error reset | errorLog |

---

## 3. BLE Callback onWrite() Code Duplication

**Lines ~778-895**: ChCallbacks::onWrite() with 12+ repeated patterns:

### Pattern Found (Kp, Ki, Kd, U1Sp, U2Sp, etc.):
```cpp
else if (uuid == UUID_KP) {
  double newKp = toF(val);
  if (isnan(newKp) || newKp < 0.1 || newKp > 100.0) {
    Serial.printf("[BLE] WARNING: Rejected invalid Kp=%.3f\n", newKp);
  } else {
    Kp = constrain(newKp, 0.1, 100.0); 
    tempPID.SetTunings(Kp, Ki, Kd);
    Serial.printf("[BLE] Kp clamped to %.3f\n", Kp);
    if (pidMode == 2) { 
      rtcU1Kp = Kp; 
      Preferences prefs; 
      prefs.begin("espatom", false); 
      char buf[32]; 
      snprintf(buf, sizeof(buf), "%.3f", rtcU1Kp); 
      prefs.putString("u1_kp", buf); 
      prefs.end(); 
      Serial.println(F("[PREF] U1 Kp saved from BLE")); 
    }
    else if (pidMode == 3) { 
      rtcU2Kp = Kp; 
      Preferences prefs; 
      prefs.begin("espatom", false); 
      char buf[32]; 
      snprintf(buf, sizeof(buf), "%.3f", rtcU2Kp); 
      prefs.putString("u2_kp", buf); 
      prefs.end(); 
      Serial.println(F("[PREF] U2 Kp saved from BLE")); 
    }
  }
}
```

**Repeat Count**: 12+ times for Kp, Ki, Kd, Default SP, U1 Sp, U2 Sp, U1 Enabled, U1 Ms, U2 Enabled, U2 Ms, Unit, Output

**Solution**: Create `PreferencesManager` class with method:
```cpp
bool setPIDParameter(const char* key, double value, double minV, double maxV);
```

---

## 4. Dead Code Identified

### 4A. Thermistor Functions (Lines ~1700-1750)
```cpp
double computeRntc() { return NAN; }  // Returns NAN — unused
double readTemperatureC() { ... }     // Uses computeRntc() → returns NAN
```
**Status**: Thermistor hardware removed but code left behind. **DELETE in Phase 5**

### 4B. RGB/ARGB Feature Removal
- RGB and ARGB status LED code paths are removed from firmware.
- Related RGB driver source files were deleted during cleanup.

**Decision**: Keep firmware LED-free unless a future hardware revision reintroduces a status LED.

### 4C. Unused Features
- Some menu items may not be fully tested (e.g., factory reset, forget bonds)
- These are not "dead" but should be verified during Phase 6 testing

---

## 5. Header Files Review

| File | Lines | Type | Issues |
|------|-------|------|--------|
| config.h | 300 | Macros only | ✅ Good — no changes needed |
| ble.h | 100 | Declarations | ✅ Good — move callbacks to .cpp |
| battery.h | 150 | **INLINE impl** | ⚠️ Should move to .cpp but keep .h interface |
| encoder.h | 200 | **INLINE impl + ISR** | ⚠️ Keep inline (performance), mark as critical |
| oled.h | 100 | **INLINE impl** | ⚠️ tryInitOLED() — consider extracting |
| ads1115_driver.h | 280 | Declarations + macros | ✅ Good — impl in .cpp |

---

## 6. Architecture Issues Summary

### High Priority (Blocking maintainability)
- [ ] **2,858 lines in single .ino** — need to split into modules
- [ ] **50+ globals scattered** — need struct organization
- [ ] **BLE callback 200+ lines with 12+ duplicate patterns** — need abstraction
- [ ] **loop() & handleSerial() 300-500 lines each** — need delegation

### Medium Priority (Code quality)
- [ ] Dead thermistor code removal
- [x] RGB/ARGB code removal verification
- [ ] Menu state organization (5+ scattered variables)
- [ ] Battery state organization (10+ scattered variables)

### Low Priority (Future refactor)
- [ ] Move battery.h impl to .cpp (keep interface)
- [ ] Move encoder.h impl to .cpp (performance tradeoff)
- [ ] Move oled.h impl to .cpp

---

## 7. Refactoring Quick Reference

### New Files to Create (Phase 2-3)
```
ESPAtomizer/StateManager.h              — structs: PIDController, MenuState, etc.
ESPAtomizer/PreferencesManager.h        — preference cache & persistence
ESPAtomizer/src/PreferencesManager.cpp
ESPAtomizer/MenuManager.h               — menu state machine
ESPAtomizer/src/MenuManager.cpp
ESPAtomizer/src/ble_manager.cpp         — BLE callbacks + setup
ESPAtomizer/src/safety_manager.cpp      — watchdog, thermal, battery, sensor checks
ESPAtomizer/src/cli_manager.cpp         — serial command handler
ESPAtomizer/src/ui_manager.cpp          — display + menu rendering
```

### Files to Modify
- ESPAtomizer.ino → reduce to ~500 lines (delegate to managers)
- config.h → verify thermistor feature flag
- ble.h → move callbacks to ble_manager.cpp

### Files to Delete
- Removed during cleanup: rgb_led_driver.cpp, addressable_rgb_driver.cpp, rgb_led_integration_example.cpp

---

## 8. Next Steps (Kickoff for Phase 2)

1. ✅ **Phase 1**: Analysis complete — document all globals & dependencies (THIS DOCUMENT)
2. ⏭️ **Phase 2**: Create state structs (StateManager.h) + PreferencesManager + MenuManager
3. ⏭️ **Phase 3**: Extract BLE, Safety, CLI, UI managers (4 parallel tasks)
4. ⏭️ **Phase 4**: Refactor setup() & loop() to delegate to managers
5. ⏭️ **Phase 5**: Remove dead code (thermistor) & cleanup headers
6. ⏭️ **Phase 6**: Compile, verify on hardware, document new architecture

---

**Analysis Document Created**: 2026-05-27  
**Next Phase Kickoff**: Ready for Phase 2 (State structs & managers)
