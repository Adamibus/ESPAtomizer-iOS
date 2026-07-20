# Phase 4D: Function Deletion Plan

**Target**: Delete all old handler functions now superseded by managers  
**Expected Removal**: ~1,010 lines  
**Result**: 2,430 → ~1,420 lines (41% total reduction)

---

## Functions to Delete (Prioritized by Dependencies)

### DELETE 1: Forward Declarations (Safe - No Dependencies)
**Location**: Lines 250-256, 345  
**Functions**:
- Line 250: `void handleSerial();`
- Line 251: `void printStatus();`
- Line 252: `void printHelp();`
- Line 253: `void logError(const char* source, const char* message);`
- Line 254: `void checkSystemHealth();`
- Line 255: `void printDiagnostics();`
- Line 256: `void resetErrorCounters();`
- Line 345: `void updateDisplay();`

**Status**: ✅ SAFE - No code calls these declarations anymore  
**Lines to remove**: 8 + empty lines

---

### DELETE 2: handleSerial() Function (227 lines)
**Location**: Lines 2041-2268  
**Code**: Serial command handler - **FULLY REPLACED BY CLIManager::update()**  
**Dependencies**: None - loop() calls `CLIManager::update()` instead  
**Lines to remove**: 227

---

### DELETE 3: updateDisplay() Function (160 lines)
**Location**: Lines 2268-2427 (includes `#if USE_OLED` and `#endif`)  
**Code**: Display rendering - **FULLY REPLACED BY UIManager::update()**  
**Dependencies**: None - loop() calls `UIManager::update()` instead  
**Lines to remove**: 160

---

### DELETE 4: ChCallbacks Class (120+ lines)
**Location**: Lines 358-484  
**Code**: BLE characteristic callbacks - **FULLY MOVED TO BLEManager.cpp**  
**Dependencies**: None - BLEManager now handles all callbacks  
**Lines to remove**: ~127

---

### DELETE 5: setupBLE() Function (100+ lines)
**Location**: Lines 485-~630  
**Code**: BLE server initialization - **FULLY MOVED TO BLEManager::init()**  
**Dependencies**: None - setup() calls `BLEManager::init()` instead  
**Lines to remove**: ~150

---

### DELETE 6: applyPidMode() Function (30+ lines)
**Location**: Line 1919  
**Code**: PID mode switching - Check if used in loop()  
**Status**: ⏳ VERIFY - Check if still called in loop() or loop calls manager  
**Lines to remove**: ~30

---

### DELETE 7: printHelp() Function (40 lines)
**Location**: Lines 2002-2040  
**Code**: Serial command help - **MOVED TO CLIManager**  
**Dependencies**: Verify not called outside handleSerial()  
**Lines to remove**: ~39

---

### DELETE 8: Old printStatus(), printDiagnostics(), logError(), checkSystemHealth(), resetErrorCounters()
**Status**: Need to search for these functions separately  
**Lines to remove**: ~100 total

---

## Deletion Sequence

1. ✅ **Phase 4D-1**: Delete forward declarations (Lines 250-256, 345)
2. ⏳ **Phase 4D-2**: Delete handleSerial() (Lines 2041-2268)
3. ⏳ **Phase 4D-3**: Delete updateDisplay() (Lines 2268-2427 including #if/#endif)
4. ⏳ **Phase 4D-4**: Delete ChCallbacks class (Lines 358-484)
5. ⏳ **Phase 4D-5**: Delete setupBLE() function
6. ⏳ **Phase 4D-6**: Delete other extracted functions
7. ⏳ **Phase 4D-7**: Verify no compilation errors

---

## Verification After Each Deletion

- [ ] File opens without errors
- [ ] No syntax issues
- [ ] No obvious broken references
- [ ] Next deletion can proceed safely

---

## Expected File Size After Phase 4D Completion

- Current: 2,430 lines
- After deletions: ~1,420 lines
- Total reduction: 2,858 → 1,420 lines (50% reduction overall)
- Remaining for Phase 5: 820 lines to reach ~600 target
