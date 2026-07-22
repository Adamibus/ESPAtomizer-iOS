#ifndef ESPATOM_STATE_MANAGER_H
#define ESPATOM_STATE_MANAGER_H

/**
 * ESPAtomizer State Manager
 * 
 * Consolidates 50+ scattered globals into logical struct groups.
 * This reduces complexity, improves maintainability, and makes state
 * dependencies explicit.
 */

#include <Arduino.h>
class PID;
class Adafruit_SSD1306;
class NimBLEServer;
class NimBLECharacteristic;

// ===== PID CONTROLLER STATE =====
struct PIDControllerState {
  // Current tuning parameters
  double Kp = 10.0;
  double Ki = 0.5;
  double Kd = 50.0;
  
  // Current operating point
  double setpointC = 200.0;
  double inputC = 0.0;      // measured temperature
  double pidOutput = 0.0;   // controller output [0 .. PWM_MAX]
  
  // Mode flags
  bool manualMode = false;  // false=PID auto, true=manual duty control
  bool systemEnabled = false;
  
  // PID mode: 0=AUTO, 1=MANUAL, 2=U1, 3=U2, 4=CONFIG
  int pidMode = 0;
  
  // Persisted values (survive deep sleep & power cycles)
  struct {
    double Kp = 10.0, Ki = 0.5, Kd = 50.0;
    double setpoint = 200.0;
    bool manualMode = false;
    bool systemEnabled = false;
    int pidMode = 0;
    double pidOutput = 0.0;
  } rtc;
  
  // U1 Preset (User Preset 1)
  struct {
    double Kp = 12.0, Ki = 0.7, Kd = 60.0;
    double setpoint = 220.0;
  } u1;
  
  // U2 Preset (User Preset 2)
  struct {
    double Kp = 8.0, Ki = 0.3, Kd = 40.0;
    double setpoint = 180.0;
  } u2;
  
  // Default setpoint (used when device boots)
  double defaultSetpoint = 200.0;
};

// ===== MENU & USER INTERFACE STATE =====
struct MenuState {
  // Main menu state
  bool active = false;           // true if menu is open
  int index = 0;                 // current menu item selection (0-4)
  
  // Configuration submenu
  bool configMode = false;       // true if editing config
  int configIndex = 0;           // config item index
  bool configEditing = false;    // true if actively editing a value
  
  // Confirmation dialog
  bool awaitingConfirmation = false;
  double pendingSetpointC = 0.0;
  unsigned long confirmationStartMs = 0;
  
  // BLE configuration (in config menu)
  struct {
    int nameIndex = 0;           // which BLE_NAME_PRESETS
    unsigned long advIntervalMs = 200;
    bool enabled = true;
    bool oledIndicator = true;
  } ble;
  
  // Temperature unit (C vs F)
  bool tempUnitIsC = true;
};

// ===== ENCODER & BUTTON INPUT STATE =====
struct InputState {
  // Button debouncing & detection
  bool stable = false;           // last stable raw level
  bool lastRead = false;         // last instantaneous raw read
  unsigned long lastChangeMs = 0;
  bool stablePressed = false;    // last stable pressed state
  unsigned long pressStartMs = 0;
  bool longHandled = false;      // whether long-press action was handled
  
  // Encoder rate limiting
  unsigned long lastEncoderInputMs = 0;
  char lastEncoderInfo[64] = "enc:none";
  
  // Manual mode hold-to-heat
  bool holdHeating = false;
  double pidOutputBeforeHold = 0;
};

// ===== BATTERY STATE =====
struct BatteryState {
  double voltage = NAN;
  int percent = -1;
  bool lowWarning = false;
  bool lowCutoff = false;
  bool chargerRemoved = false;
  bool chargerRampActive = false;
  
  // Trend detection for charger removal
  double voltagePrevious = 0.0;
  unsigned long voltageCheckMs = 0;
  unsigned long chargerRampStartMs = 0;
  
  // Hysteresis
  bool hysteresisActive = false;
};

// ===== SENSOR FAULT STATE =====
struct SensorFaultState {
  bool faulted = false;
  int faultCount = 0;             // consecutive invalid readings
  int validCount = 0;             // consecutive valid readings
  unsigned long faultMs = 0;
};

// ===== WATCHDOG & SAFETY STATE =====
struct SafetyState {
  // Watchdog
  bool watchdogFaulted = false;
  unsigned long watchdogLastLoopMs = 0;
  unsigned long pidLastComputeMs = 0;
  
  // Thermal runaway
  bool thermalRunawayFaulted = false;
  unsigned long thermalRunawayMs = 0;
  unsigned long maxOnTimeStartMs = 0;
  float lastAppliedOutput = 0.0;
  
  // Continuous run limit
  unsigned long continuousRunStartMs = 0;
  bool cooldownRequired = false;
  unsigned long cooldownStartMs = 0;
};

// ===== SYSTEM HEALTH MONITORING =====
struct SystemHealthState {
  bool operational = true;
  bool sensorOk = true;
  bool heaterOk = true;
  bool batteryOk = true;
  bool bleOk = true;
  bool temperatureInRange = true;
  bool noFaults = true;
  unsigned long lastHealthCheck = 0;
  char statusMessage[64] = "System OK";
};

// ===== ERROR LOGGING =====
struct ErrorLogState {
  int totalErrors = 0;
  int sensorErrors = 0;
  int heaterErrors = 0;
  int batteryErrors = 0;
  int bleErrors = 0;
  int thermalRunawayEvents = 0;
  unsigned long lastErrorMs = 0;
  char lastError[64] = "None";
};

// ===== BOOT SELF-TEST STATE =====
struct BootTestState {
  bool sensorOk = true;
  bool heaterOk = true;
  bool encoderOk = true;
  bool buttonOk = true;
  bool batteryOk = true;
};

// ===== BLE GLOBAL STATE =====
struct BLEState {
  NimBLEServer* server = nullptr;
  
  // Characteristics
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
  NimBLECharacteristic *chTcStatus = nullptr;
  NimBLECharacteristic *chStatus = nullptr;   // write-result ack ("OK:<FIELD>" / "ERR:<FIELD>:<reason>")

  // Animation state
  unsigned long animExpireMs = 0;
  bool animIsConnect = false;
  char lastEvent[32] = "none";

  // Last power/setpoint/output PUBLISHED to the app. The loop() watcher
  // (ESPAtomizer.ino) notifies only when live state drifts from these, and the
  // BLE write callback updates them on every accepted app write — so device-side
  // changes (dial, safety shutoff, presets) are pushed, while app-originated
  // writes are never echoed back (a late echo can race a slider mid-drag and
  // snap it backwards).
  bool   pubEnableSeeded = false;
  bool   pubEnable = false;
  double pubSetpoint = NAN;
  int    pubOut = -1;
};

// ===== OLED/DISPLAY STATE =====
struct DisplayState {
  Adafruit_SSD1306* display = nullptr;
  bool available = false;
  int failureCount = 0;
  unsigned long lastSuccessMs = 0;
};

// ===== DIAGNOSTIC COUNTERS =====
struct DiagnosticsState {
  unsigned long loopIterations = 0;
  unsigned long pidComputations = 0;
  unsigned long outputChanges = 0;
  unsigned long bleWrites = 0;
  unsigned long sensorReads = 0;
  unsigned long sensorReadsFailed = 0;
};

// ===== TIMING & THROTTLING =====
struct TimingState {
  unsigned long loopIntervalMs = 1000;
  unsigned long lastLoopMs = 0;
  unsigned long lastTempSampleMs = 0;
  unsigned long lastTempPrintMs = 0;
  unsigned long lastActivityMs = 0;
  unsigned long lastDebugMs = 0;
  unsigned long lastSleepPrintMs = 0;
  unsigned long lastApplyPrintMs = 0;
  int lastAppliedDuty = -1;
  
  // Relay mode window timing
  unsigned long windowStartTime = 0;
};

// ===== FEATURE FLAGS =====
struct FeatureFlags {
  bool remoteControlEnabled = true;
  bool serialStreamingEnabled = true;
  bool deepSleepEnabled = true;
  bool deepSleepDisabledRuntime = false;
};

// ===== CONSOLIDATED STATE CONTAINER =====
/**
 * Global state manager that consolidates all 50+ scattered globals
 * into organized, typed structures. This improves:
 * - Readability: State grouped by concern
 * - Testability: Dependencies explicit via struct references
 * - Maintainability: Easy to add/remove state without scattered changes
 * - Initialization: All state in one place, clear reset/boot logic
 */
class GlobalState {
public:
  PIDControllerState pidController;
  MenuState menu;
  InputState input;
  BatteryState battery;
  SensorFaultState sensorFault;
  SafetyState safety;
  SystemHealthState systemHealth;
  ErrorLogState errorLog;
  BootTestState bootTest;
  BLEState ble;
  DisplayState display;
  DiagnosticsState diagnostics;
  TimingState timing;
  FeatureFlags features;
  
  // PID object (from library)
  PID* tempPID = nullptr;
  
  // Hardware object references
  void* ads1115Instance = nullptr;  // ads1115 sensor object
  
  /**
   * Reset state to safe defaults
   * Called on boot or manual reset
   */
  void reset() {
    pidController = PIDControllerState();
    menu = MenuState();
    input = InputState();
    battery = BatteryState();
    sensorFault = SensorFaultState();
    safety = SafetyState();
    systemHealth = SystemHealthState();
    errorLog = ErrorLogState();
    bootTest = BootTestState();
    diagnostics = DiagnosticsState();
    timing = TimingState();
    features = FeatureFlags();
  }
  
  /**
   * Restore persisted state from RTC memory
   * Called after deep sleep to resume previous operation
   */
  void restoreFromRTC() {
    pidController.Kp = pidController.rtc.Kp;
    pidController.Ki = pidController.rtc.Ki;
    pidController.Kd = pidController.rtc.Kd;
    pidController.setpointC = pidController.rtc.setpoint;
    pidController.manualMode = pidController.rtc.manualMode;
    pidController.systemEnabled = pidController.rtc.systemEnabled;
    pidController.pidMode = pidController.rtc.pidMode;
    pidController.pidOutput = pidController.rtc.pidOutput;
  }
  
  /**
   * Snapshot current state to RTC memory for persistence
   * Called before deep sleep
   */
  void saveToRTC() {
    pidController.rtc.Kp = pidController.Kp;
    pidController.rtc.Ki = pidController.Ki;
    pidController.rtc.Kd = pidController.Kd;
    pidController.rtc.setpoint = pidController.setpointC;
    pidController.rtc.manualMode = pidController.manualMode;
    pidController.rtc.systemEnabled = pidController.systemEnabled;
    pidController.rtc.pidMode = pidController.pidMode;
    pidController.rtc.pidOutput = pidController.pidOutput;
  }
};

#endif // ESPATOM_STATE_MANAGER_H
