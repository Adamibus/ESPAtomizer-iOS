# Phase 4: ESPAtomizer.ino Refactoring Guide

**Status**: In Progress  
**Objective**: Integrate managers and reduce .ino from 2,858 → ~600 lines  
**Date**: 2026-05-27

---

## Phase 4 Implementation Strategy

Rather than attempting fragile line-by-line replacements on a 2,858 line file, this guide provides a **structural refactoring approach** that's safe, verifiable, and maintainable.

### High-Level Changes Required

1. ✅ **Add manager includes** (DONE)
2. ✅ **Add GlobalState instance** (DONE)
3. ⏳ **Refactor setup()** - Replace inline code with manager calls
4. ⏳ **Refactor loop()** - Delegate to managers
5. ⏳ **Delete extracted functions** - Remove old code
6. ⏳ **Verify compilation** - Ensure no breaking changes

---

## What Needs to Be Done

### **Section A: Remove Old BLE Globals** (Lines ~132-160)

**DELETE** (now in GlobalState.ble):
```cpp
#if USE_BLE
// Single definitions for BLE globals declared `extern` in `ble.h`
NimBLEServer* bleServer = nullptr;
NimBLECharacteristic *chEnable = nullptr;
NimBLECharacteristic *chSetpoint = nullptr;
NimBLECharacteristic *chKp = nullptr;
NimBLECharacteristic *chKi = nullptr;
NimBLECharacteristic *chKd = nullptr;
NimBLECharacteristic *chMode = nullptr;
NimBLECharacteristic *chTemp = nullptr;
NimBLECharacteristic *chOut = nullptr;
NimBLECharacteristic *chBat = nullptr;
NimBLECharacteristic *chModeRead = nullptr;
NimBLECharacteristic *chDefaultSp = nullptr;
NimBLECharacteristic *chUnit = nullptr;
NimBLECharacteristic *chPreU1Enabled = nullptr;
NimBLECharacteristic *chPreU1Ms = nullptr;
NimBLECharacteristic *chPreU2Enabled = nullptr;
NimBLECharacteristic *chPreU2Ms = nullptr;
NimBLECharacteristic *chTcStatus = nullptr;

char lastBleEvent[32] = "none";
#endif

char lastEncoderInfo[64] = "enc:none";
```

**KEEP**: Only the firmware version line
```cpp
static const char* FIRMWARE_VERSION = "ESPAtomizer v0.1";
```

---

### **Section B: Remove Old OLED/Battery/PID Globals** (Lines ~175-450)

**DELETE ALL** of these sections:
- Old `Adafruit_SSD1306 display` + `displayAvailable`
- Old `batteryVoltage`, `batteryPercent`, `batteryLow` + all battery safety variables
- Old `Kp`, `Ki`, `Kd`, `setpointC`, `inputC`, `pidOutput` 
- Old `tempPID` object
- All RTC_DATA_ATTR variables (rtcSetpointC, rtcKp, rtcKi, etc.)
- `SystemHealth` struct and all members
- `ErrorLog` struct and all members
- All `loopIntervalMs`, `manualMode`, `menuActive`, `menuIndex` globals
- All button/encoder/menu state variables
- All battery fault/charger variables
- All boot self-test flags
- All watchdog/thermal/safety state variables

**KEEP ONLY**:
```cpp
// PWM configuration (used directly)
static const uint16_t PWM_MAX = (1U << PWM_RES_BITS) - 1U;
static_assert(PWM_MAX > 0, "CRITICAL: PWM_MAX is zero!");
static const int PWM_CHANNEL = 0;

// PID object (library dependency)
static PID tempPID(&gState.pidController.inputC, 
                   &gState.pidController.pidOutput, 
                   &gState.pidController.setpointC, 
                   10.0, 0.5, 50.0, DIRECT);

// Sensor object (hardware driver)
ADS1115_SENSOR ads1115Instance;  // or equivalent sensor object
```

---

### **Section C: Remove Old Function Declarations** (Lines ~500-530)

**DELETE** (now in managers):
```cpp
void applyOutput(double value);
void applyRelayWindow(double value);
void handleSerial();
void printStatus();
void printHelp();
void logError(const char* source, const char* message);
void checkSystemHealth();
void printDiagnostics();
void resetErrorCounters();
void applyPidMode(int mode);
```

**KEEP** (hardware-specific):
```cpp
void applyOutput(double value);  // Still needed in main .ino
void applyRelayWindow(double value);  // Still needed in main .ino
void wifiDoSetup();
void wifiLoopHandle();
```

---

### **Section D: Remove Old BLE Implementation** (Lines ~778-1077)

**DELETE ENTIRE SECTIONS**:
- `class ChCallbacks : public NimBLECharacteristicCallbacks` (200 lines)
- `void setupBLE()` function (200 lines)
- Both are now in `BLEManager::init()` and internal callbacks

---

### **Section E: Refactor setup()** (Lines ~1080-1400)

**OLD STRUCTURE** (~350 lines):
```cpp
void setup() {
  Serial.begin();
  // ... 20+ inline initialization calls ...
  // Load preferences manually
  Preferences prefs;
  prefs.begin("espatom", true);
  String v = prefs.getString("def_sp", "");
  if (v.length()) rtcDefaultSetpoint = atof(v.c_str());
  // ... 15+ more preference loads ...
  prefs.end();
  
  // Initialize subsystems inline
  initBattery();
  encoderInit();
  tryInitOLED();
  // ... more inline init ...
  
  pinMode(OUTPUT_PIN, OUTPUT);
  // ... more setup ...
  
  // Boot self-tests inline
  runBootSelfTests();
  
  // BLE setup
  setupBLE();
  
  // WiFi setup
  wifiDoSetup();
}
```

**NEW STRUCTURE** (~150 lines):
```cpp
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  while (!Serial && millis() < 3000) delay(10);
  
  Serial.println(F("[Setup] Starting ESPAtomizer..."));
  
  // Initialize base managers (no dependencies)
  PreferencesManager::init();
  MenuManager::init();
  
  // Initialize global state
  gState.reset();
  
  // Load persisted configuration from NVS
  gState.pidController.defaultSetpoint = PreferencesManager::getDefaultSetpoint();
  gState.menu.tempUnitIsC = PreferencesManager::getTempUnitC();
  gState.menu.ble.enabled = PreferencesManager::getBleEnabled();
  gState.menu.ble.nameIndex = PreferencesManager::getBleName();
  
  // Initialize hardware
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  #if USE_BAT
  initBattery();
  sampleBattery();
  #endif
  
  #if USE_ENCODER
  encoderInit();
  #endif
  
  #if USE_OLED
  tryInitOLED();
  #endif
  
  // Initialize PWM
  pwmInit();
  
  // Initialize PID controller
  tempPID.SetMode(AUTOMATIC);
  tempPID.SetOutputLimits(0, PWM_MAX);
  tempPID.SetSampleTime(1000);
  
  // Initialize subsystem managers (order matters)
  SafetyManager::init(gState);
  
  #if USE_BLE
  BLEManager::init(gState);
  #endif
  
  CLIManager::init(gState);
  
  #if USE_OLED
  UIManager::init(gState);
  #endif
  
  // WiFi (if enabled)
  wifiDoSetup();
  
  Serial.println(F("[Setup] Complete"));
}
```

---

### **Section F: Refactor loop()** (Lines ~1600-1900)

**OLD STRUCTURE** (~300 lines):
```cpp
void loop() {
  unsigned long now = millis();
  
  // Watchdog check inline
  if (now - watchdogLastLoopMs > WATCHDOG_TIMEOUT_MS) {
    watchdogFaulted = true;
    // ...
  }
  
  // Check thermal runaway inline
  if (inputC > THERMAL_RUNAWAY_TEMP_C) {
    thermalRunawayFaulted = true;
    // ...
  }
  
  // Check battery inline
  // ... 20+ lines of battery logic ...
  
  // Check sensor faults inline
  // ... 15+ lines of sensor logic ...
  
  // Button debounce inline
  // ... 20+ lines of button logic ...
  
  // Encoder handling inline
  // ... 15+ lines of encoder logic ...
  
  // Menu logic inline
  if (menuActive) {
    if (encoderMovedRight) menuIndex++;
    // ... 10+ lines of menu state ...
  }
  
  // Sensor reading
  inputC = readTemperatureC();
  
  // PID compute
  tempPID.Compute();
  
  // Apply output
  applyOutput(pidOutput);
  
  // Display update inline
  updateDisplay();
  
  // Serial handling
  handleSerial();
  
  // Sleep logic
  // ... 20+ lines of deep sleep ...
}
```

**NEW STRUCTURE** (~150 lines):
```cpp
void loop() {
  unsigned long now = millis();
  
  // Mark loop entry for watchdog
  SafetyManager::markLoopEntry();
  
  // ===== INPUT HANDLING =====
  // Button debounce & encoder reading (external functions)
  handleButtonInput(now);
  handleEncoderInput(now);
  
  // ===== SENSOR READING (throttled) =====
  #if USE_ADS1115
  if (now - lastTempSampleMs >= MIN_TEMP_SAMPLE_MS) {
    double tempC = readTemperatureC();
    bool valid = !isnan(tempC);
    gState.pidController.inputC = tempC;
    SafetyManager::markSensorRead(tempC, valid);
    lastTempSampleMs = now;
  }
  #endif
  
  // ===== PID COMPUTATION =====
  if (gState.pidController.systemEnabled && SafetyManager::isSafe()) {
    tempPID.Compute();
    SafetyManager::markPIDCompute();
  }
  
  // ===== SAFETY CHECKS =====
  SafetyManager::update();
  
  // ===== APPLY OUTPUT =====
  if (!SafetyManager::isSafe()) {
    applyOutput(0);  // Kill heater on fault
  } else {
    applyOutput(gState.pidController.pidOutput);
  }
  
  // ===== DISPLAY UPDATE (throttled) =====
  #if USE_OLED
  if (now - lastDisplayMs >= DISPLAY_UPDATE_MS) {
    UIManager::update();
    lastDisplayMs = now;
  }
  #endif
  
  // ===== SERIAL CLI =====
  CLIManager::update();
  
  // ===== BATTERY SAMPLING (throttled) =====
  #if USE_BAT
  if (now - lastBatterySampleMs >= BAT_SAMPLE_INTERVAL_MS) {
    sampleBattery();
    gState.battery.voltage = batteryVoltage;
    gState.battery.percent = batteryPercent;
    lastBatterySampleMs = now;
  }
  #endif
  
  // ===== DEEP SLEEP =====
  if (SLEEP_ON_IDLE_MS > 0 && !DISABLE_DEEP_SLEEP && 
      !gState.menu.active && !gState.pidController.systemEnabled &&
      now - lastActivityMs > SLEEP_ON_IDLE_MS) {
    Serial.println(F("[Sleep] Entering deep sleep..."));
    gState.saveToRTC();
    esp_deep_sleep(SLEEP_ON_IDLE_MS * 1000);
  }
  
  // ===== WIFI LOOP =====
  wifiLoopHandle();
}
```

---

### **Section G: Keep Helper Functions** (modify slightly)

**KEEP & UPDATE**:
```cpp
// PWM wrappers (unchanged)
static inline void pwmInit() { ... }
static inline void pwmWrite(int duty) { ... }

// Sensor reading (unchanged)
double readTemperatureC() { ... }

// Output application (keep but simplify)
void applyOutput(double value) {
  gState.pidController.pidOutput = value;
  pwmWrite((int)value);
  gState.diagnostics.outputChanges++;
}

// Input handlers (keep but move non-menu logic to managers)
void handleButtonInput(unsigned long now) {
  // Button debounce logic
  // Menu navigation calls MenuManager
  // Confirmation dialog calls MenuManager
}

void handleEncoderInput(unsigned long now) {
  // Encoder reading
  // Rate limiting
  // Menu navigation via MenuManager
}
```

---

### **Section H: Delete OLD Functions** (Entire functions to remove)

**DELETE** (~500-1000 lines total):
- `void handleSerial()` (500+ lines) → Now in CLIManager
- `void updateDisplay()` (200+ lines) → Now in UIManager
- `void checkWatchdog()` → Now in SafetyManager
- `void checkThermalRunaway()` → Now in SafetyManager
- `void checkBatterySafety()` → Now in SafetyManager
- `void updateSensorFaultState()` → Now in SafetyManager
- `void runBootSelfTests()` → Can keep or move to separate file
- `void logError()` → Simplify, error logging now in managers
- `void checkSystemHealth()` → Now in SafetyManager
- `void printDiagnostics()` → Now in CLIManager
- `void resetErrorCounters()` → Now in SafetyManager
- `void applyPidMode()` → Can be simplified for quick mode switching
- `void applyRelayWindow()` (if RELAY_MODE not used) → Delete
- `void applyOutputWithRampLimit()` → Can delete if ramp logic in applyOutput

---

## Timeline & Verification

### Expected Line Counts
| Section | Before | After | Reduction |
|---------|--------|-------|-----------|
| Includes | 20 | 20 | - |
| Globals | 400 | 50 | 87% |
| setup() | 350 | 150 | 57% |
| loop() | 300 | 150 | 50% |
| Helper functions | 500 | 200 | 60% |
| Deleted functions | 1,300 | 0 | 100% |
| **Total** | **2,858** | **~600** | **79%** |

---

## Compilation Checkpoints

After each major section:
1. ✅ Add includes and GlobalState → Should compile
2. ✅ Delete old globals → Should compile
3. ✅ Refactor setup() → Should compile & initialize all managers
4. ✅ Refactor loop() → Should compile & run main loop
5. ✅ Delete old functions → Should compile with no undefined symbols

---

## Next Steps

This guide provides the **roadmap** for Phase 4. The actual implementation should:
1. Start with deletion of old globals (safest first)
2. Refactor setup() and verify manager initialization
3. Refactor loop() to use manager API calls
4. Delete old extracted functions
5. Compile and test on hardware

**Would you like me to proceed with implementing these changes using targeted replacements?**
