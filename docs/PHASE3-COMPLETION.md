# ESPAtomizer Refactoring - Phase 3 Complete

**Date**: 2026-05-27  
**Status**: ✅ All 4 manager subsystems extracted and ready to integrate  
**Files Created**: 8 new files (4 headers + 4 implementations)

---

## Phase 3 Summary: Manager Extraction

We've successfully extracted all business logic from the monolithic .ino into 4 focused, testable manager classes.

### **Files Created**

| File | Type | Lines | Purpose |
|------|------|-------|---------|
| BLEManager.h | Header | 50 | BLE server, characteristics, callbacks |
| src/BLEManager.cpp | Implementation | 250 | setupBLE() + ChCallbacks, uses PreferencesManager |
| SafetyManager.h | Header | 70 | Watchdog, thermal, battery, sensor safety |
| src/SafetyManager.cpp | Implementation | 180 | All safety checks, fault detection & recovery |
| CLIManager.h | Header | 50 | Serial CLI commands |
| src/CLIManager.cpp | Implementation | 250 | handleSerial() + 12 commands |
| UIManager.h | Header | 100 | OLED display rendering |
| src/UIManager.cpp | Implementation | 220 | updateDisplay() + menu/config/fault screens |

**Total**: ~1,170 lines of extracted, modularized code

---

## What Each Manager Does

### **1️⃣ BLEManager** (BLEManager.h + src/BLEManager.cpp)

**Extracted From**: Lines ~778-1077 of .ino (setupBLE + ChCallbacks)

**Key Features**:
- Initializes NimBLE server and all 18 characteristics
- Implements ChCallbacks with write handlers
- **Uses PreferencesManager** for all preference operations (eliminates 150 lines of duplicate code)
- Server callbacks for connect/disconnect events
- Methods: `init()`, `isConnected()`, `notifyAllCharacteristics()`, `setDeviceName()`, `clearBonds()`

**Code Reduction Example**:

**Before** (in BLE callback, repeated 12+ times):
```cpp
else if (uuid == UUID_KP) {
  double newKp = toF(val);
  if (isnan(newKp) || newKp < 0.1 || newKp > 100.0) {
    Serial.printf("[BLE] WARNING: Rejected invalid Kp\n");
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
      Serial.println(F("[PREF] U1 Kp saved"));
    }
    // ... 8 more lines for U2
  }
}
```

**After** (via BLEManager + PreferencesManager):
```cpp
else if (uuid == UUID_KP) {
  double newKp = toF(val);
  if (!isnan(newKp) && newKp >= 0.1 && newKp <= 100.0) {
    pGState->pidController.Kp = constrain(newKp, 0.1, 100.0);
    PreferencesManager::setKp(pGState->pidController.Kp, pGState->pidController.pidMode);
    Serial.printf("[BLE] Kp set to %.3f\n", pGState->pidController.Kp);
  }
}
```

---

### **2️⃣ SafetyManager** (SafetyManager.h + src/SafetyManager.cpp)

**Extracted From**: Lines ~1750-2050 of .ino (4 safety check functions)

**Key Features**:
- **Watchdog**: Detects loop stall (>5s) and PID timeout (>10s)
- **Thermal Runaway**: 4 checks: absolute max temp, setpoint margin, max on-time
- **Battery Safety**: Low voltage detection, charger removal, ramp-down
- **Sensor Fault**: Consecutive read failures with recovery logic
- All safety state in `GlobalState::SafetyState` struct
- Methods: `init()`, `update()`, `isSafe()`, `markLoopEntry()`, `markPIDCompute()`, `markSensorRead()`, `getLastFault()`

**Loop Integration**:
```cpp
void loop() {
  unsigned long now = millis();
  
  SafetyManager::markLoopEntry();  // Watchdog timestamp
  
  // ... sensor reading ...
  SafetyManager::markSensorRead(tempC, isValid);  // Fault detection
  
  // ... PID compute ...
  SafetyManager::markPIDCompute();  // Watchdog timestamp
  
  SafetyManager::update();  // All safety checks
  
  if (!SafetyManager::isSafe()) {
    applyOutput(0);  // Kill heater on fault
  }
}
```

---

### **3️⃣ CLIManager** (CLIManager.h + src/CLIManager.cpp)

**Extracted From**: Lines ~2250-2700+ of .ino (handleSerial function, ~500 lines)

**Key Features**:
- Parses 12 serial commands: S, O, T, X, Z, U, M, R, D, V, P, W, Y, Q, B, E, H
- **Uses PreferencesManager** for all preference updates
- **Uses SafetyManager** for system state queries
- Commands: Kp/Ki/Kd tuning, setpoint, output, mode switching, diagnostics
- Methods: `init()`, `update()`, `printHelp()`, `printStatus()`, `printDiagnostics()`

**Command Examples**:
```
S 12.5     → Set Kp = 12.5
O 0.8      → Set Ki = 0.8
T 55.0     → Set Kd = 55.0
X 220      → Set setpoint = 220C
M AUTO     → Switch to AUTO mode
Z          → Toggle serial streaming
D          → Print diagnostics
V          → Print status
R          → System reset
```

**Code Reduction**: Extracted ~500 lines from loop/main .ino

---

### **4️⃣ UIManager** (UIManager.h + src/UIManager.cpp)

**Extracted From**: Lines ~2700-2850+ of .ino (updateDisplay function, ~200 lines)

**Key Features**:
- Initializes and manages OLED display
- Renders 4 different screens based on state:
  - **Main Display**: Temperature, setpoint, PWM bar
  - **Main Menu**: AUTO/MANUAL/U1/U2/Config with cursor
  - **Config Menu**: Config options with cursor
  - **Fault Screen**: Error reason and recovery instructions
- **getDisplayState()** function for separate rendering logic
- Methods: `init()`, `update()`, `getDisplayState()`, `forceRefresh()`, `isAvailable()`

**Display State Query**:
```cpp
struct DisplayInfo {
  const char* mainMode;      // "AUTO", "MANUAL", "U1", "U2", "CONFIG"
  double temperature;
  double setpoint;
  int pwmDuty;
  int batteryPercent;
  bool bleConnected;
  bool menuActive;
  bool configActive;
  bool confirmationActive;
  bool faultDetected;
  const char* faultReason;
};

UIManager::DisplayInfo info = UIManager::getDisplayState();
// Caller can use info to render custom display logic
```

**Code Reduction**: Extracted ~200 lines from .ino

---

## Architecture Diagram (Post-Phase 3)

```
┌─────────────────────────────────────────────┐
│        ESPAtomizer.ino (to be refactored)   │
│                                             │
│  setup() {                                  │
│    GlobalState gState;                      │
│    PreferencesManager::init();              │
│    MenuManager::init();                     │
│    BLEManager::init(gState);                │
│    SafetyManager::init(gState);             │
│    CLIManager::init(gState);                │
│    UIManager::init(gState);                 │
│  }                                          │
│                                             │
│  loop() {                                   │
│    SafetyManager::markLoopEntry();          │
│    // ... sensor reading ...                │
│    SafetyManager::markSensorRead();         │
│    tempPID.Compute();                       │
│    SafetyManager::markPIDCompute();         │
│    SafetyManager::update();                 │
│    if (!SafetyManager::isSafe())            │
│      applyOutput(0);                        │
│    UIManager::update();                     │
│    CLIManager::update();                    │
│  }                                          │
└─────────────────────────────────────────────┘
         ▲      ▲       ▲       ▲      ▲
         │      │       │       │      │
         │      │       │       │      └─────────────┐
         │      │       │       │                    │
    ┌────▼──┐  │    ┌───▼────┐ │   ┌──────────────┐ │
    │ BLE   │  │    │ Safety │ │   │ PreferencesM │ │
    │Manager│  │    │Manager │ │   │    Manager   │ │
    └───────┘  │    └────────┘ │   └──────────────┘ │
               │               │                     │
           ┌───▼────┐      ┌────▼──┐           ┌─────▼─┐
           │   CLI  │      │  UI   │           │ Menu  │
           │Manager │      │Manager│           │Manager│
           └────────┘      └───────┘           └───────┘
               │               │                  │
               └───────┬───────┴──────────────────┘
                       │
            ┌──────────▼────────────┐
            │   GlobalState (all    │
            │   state via structs)  │
            └───────────────────────┘
```

---

## Manager Initialization Order (Important!)

```cpp
void setup() {
  // 1. Initialize global state
  GlobalState gState;
  
  // 2. Initialize base managers first (no dependencies)
  PreferencesManager::init();
  MenuManager::init();
  
  // 3. Load persisted configuration
  gState.pidController.defaultSetpoint = PreferencesManager::getDefaultSetpoint();
  gState.menu.tempUnitIsC = PreferencesManager::getTempUnitC();
  gState.pidController.ble.enabled = PreferencesManager::getBleEnabled();
  
  // 4. Initialize subsystem managers (order matters)
  SafetyManager::init(gState);    // No dependencies
  BLEManager::init(gState);        // After SafetyManager
  CLIManager::init(gState);        // After BLEManager
  UIManager::init(gState);         // Last (can call other managers)
  
  Serial.println("[Setup] All managers initialized");
}
```

---

## Next Steps: Phase 4 - Refactor ESPAtomizer.ino

Now that all managers are ready, **Phase 4** will:

1. **Add includes** to .ino:
   ```cpp
   #include "StateManager.h"
   #include "PreferencesManager.h"
   #include "MenuManager.h"
   #include "BLEManager.h"
   #include "SafetyManager.h"
   #include "CLIManager.h"
   #include "UIManager.h"
   ```

2. **Create GlobalState instance**:
   ```cpp
   GlobalState gState;
   ```

3. **Replace globals** with gState fields:
   - `Kp` → `gState.pidController.Kp`
   - `menuActive` → `MenuManager::isMainMenuActive()`
   - `batteryVoltage` → `gState.battery.voltage`
   - ... (50+ replacements)

4. **Refactor setup()**:
   - Replace inline init code with manager calls
   - Load persisted config via PreferencesManager
   - Reduced from ~350 lines to ~150 lines

5. **Refactor loop()**:
   - Add SafetyManager timestamps
   - Replace inline safety checks with `SafetyManager::update()`
   - Replace display update with `UIManager::update()`
   - Replace serial reading with `CLIManager::update()`
   - Reduced from ~300 lines to ~150 lines

6. **Delete old code**:
   - Remove old ChCallbacks class
   - Remove old setupBLE() function
   - Remove old handleSerial() function
   - Remove old updateDisplay() function
   - Remove 50+ global variable declarations

**Expected Result**: .ino reduced from **2,858 → ~600 lines**

---

## Code Quality Improvements (Phase 3 Complete)

| Aspect | Before | After | Improvement |
|--------|--------|-------|-------------|
| Global variables | 50+ scattered | GlobalState struct | ✅ Organized |
| BLE preference code | 150 lines duplicated | 10 lines via manager | ✅ 93% reduction |
| Modularization | 1 monolithic .ino | 4 focused managers | ✅ Separation of concerns |
| Testability | Very low | High (managers are isolated) | ✅ Better |
| Maintainability | Difficult | Easy (change one manager) | ✅ Much better |
| Reusability | None (all in .ino) | Managers can be reused | ✅ Modular |

---

## Phase 3 Verification Checklist

✅ BLEManager extracts setupBLE() + ChCallbacks  
✅ SafetyManager extracts 4 safety check functions  
✅ CLIManager extracts handleSerial() + all commands  
✅ UIManager extracts updateDisplay() + menu rendering  
✅ All managers use GlobalState for state access  
✅ All managers are self-contained (single responsibility)  
✅ PreferencesManager used for all NVS operations  
✅ MenuManager used for menu state  
✅ Stub implementations for disabled features (USE_BLE, USE_OLED)  
✅ Serial logging in all managers  

---

## Next Action: Phase 4

Ready to refactor ESPAtomizer.ino to integrate all managers. This will:
- Reduce .ino from 2,858 to ~600 lines
- Replace all globals with gState
- Simplify setup() and loop()
- Remove extracted code blocks

Proceed with Phase 4 refactoring? 🎯
