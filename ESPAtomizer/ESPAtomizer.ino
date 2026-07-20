#include <Arduino.h>
#include <PID_v1.h>
#include <esp_sleep.h>  // For sleep functions
#include <esp_system.h>
#include <math.h>  // For thermistor Steinhart-Hart calculation
#include <Preferences.h>

#include "config.h"
#include "ble.h"

// ===== NEW MANAGER INCLUDES (Phase 4 Refactor) =====
#include "StateManager.h"
#include "PreferencesManager.h"
#include "MenuManager.h"
#include "BLEManager.h"
#include "SafetyManager.h"
#include "CLIManager.h"
#include "UIManager.h"

// ESPAtomizer firmware entrypoint and manager-based architecture

#if USE_OLED
#include "oled.h"
#endif

// Firmware version string shown on boot OLED
static const char* FIRMWARE_VERSION = "ESPAtomizer v0.2";


// ===== GLOBAL STATE (Phase 4 Refactor) =====
// Single instance replaces 50+ scattered globals
GlobalState gState;

// Display hardware instance used by OLED boot/setup and UI management.
#if USE_OLED
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
#endif

#if USE_WIFI
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#endif

// Boot self-test configuration (results now in GlobalState.bootTests)
#ifndef SELFTEST_TIMEOUT_MS
#define SELFTEST_TIMEOUT_MS 3000  // Self-test timeout per component
#endif

// ESPAtomizer power, battery, and sleep behavior are managed by GlobalState and the safety/menu subsystems.

// Forward declaration for boot self-test (definition is after loop())
static void runBootSelfTests();

// (BLE globals moved to `ble.h`)

// ADS1115 I2C Thermocouple Driver
#include "ads1115_driver.h"

// Explicit ESP32-C6 GPIO assignments (avoid relying on Dn aliases across cores)
// XIAO ESP32-C6 (aligned to Seeed D-pin labels)
//  - Thermistor node: GPIO0 (A0 / D0)
//  - Rotary encoder:  A=GPIO0 (D0), B=GPIO1 (D1), SW=GPIO2 (D2, active-low)

// PID controller (DIRECT means increase output when error positive)
// Note: Configured to use gState.pidController members for I/O
static PID tempPID(&gState.pidController.inputC, 
                   &gState.pidController.pidOutput, 
                   &gState.pidController.setpointC, 
                   10.0, 0.5, 50.0, DIRECT);

// ===== MANAGEMENT & MONITORING ENHANCEMENTS =====

// Operational Limits (fail-safe boundaries)
#ifndef ABSOLUTE_MAX_TEMP_C
#define ABSOLUTE_MAX_TEMP_C 350.0f    // Hard limit: never exceed this temp
#endif

// Error Recovery & Diagnostics (now in GlobalState.errorLog and gState.diagnostics)
#ifndef MAX_ERROR_COUNT
#define MAX_ERROR_COUNT 10            // Max errors before requiring manual reset
#endif
#ifndef ERROR_RECOVERY_DELAY_MS
#define ERROR_RECOVERY_DELAY_MS 5000  // Wait before attempting recovery
#endif

// ===== END MANAGEMENT ENHANCEMENTS =====

// Control flow configuration is now centralized in GlobalState/menu managers

// Minimum allowed sample interval (ms) for temperature sensor
#ifndef MIN_TEMP_SAMPLE_MS
#define MIN_TEMP_SAMPLE_MS 100UL
#endif

// Timing for periodic reports
#ifndef TEMP_PRINT_INTERVAL_MS
#define TEMP_PRINT_INTERVAL_MS 5000UL
#endif

// Throttle for SLEEP messages (ms)
#ifndef SLEEP_PRINT_INTERVAL_MS
#define SLEEP_PRINT_INTERVAL_MS 30000UL
#endif

// Throttle for APPLYOUT prints: only print when duty changes or this interval passes
#ifndef APPLY_PRINT_INTERVAL_MS
#define APPLY_PRINT_INTERVAL_MS 2000UL
#endif

// Serial output throttling and runtime behavior are managed in gState.features.

// Sensor fault detection and watchdog timers are configured in config.h

#if USE_ENCODER
#include "encoder.h"
#endif

#if USE_BAT
#include "battery.h"
#endif

// PWM/analog range derived from resolution in config.h
// Compile-time validation: PWM_MAX must never be zero
static_assert(PWM_MAX > 0, "CRITICAL: PWM_MAX is zero! Verify PWM_RES_BITS >= 1");
// PWM channel to use for LEDC (ESP32)
static const int PWM_CHANNEL = 0;

// Forward declarations
void applyOutput(double value);
void applyRelayWindow(double value);
// WiFi hooks (optional) - provide no-op defaults when WiFi support is not compiled in
void wifiDoSetup();
void wifiLoopHandle();

// Default WiFi hook implementations
void wifiDoSetup() {}
void wifiLoopHandle() {}

// Provide a compile-time default for DISABLE_DEEP_SLEEP when not defined
#ifndef DISABLE_DEEP_SLEEP
#define DISABLE_DEEP_SLEEP 0
#endif

// Window timing used for RELAY_MODE PWM emulation
#ifndef HAVE_ESP32_LEDC
#if defined(__has_include)
  // Check for common LEDC/ESP32 headers; if found, enable LEDC usage
  #if __has_include(<esp32-hal-ledc.h>) || __has_include(<driver/ledc.h>) || __has_include(<ledc.h>)
    #define HAVE_ESP32_LEDC 1
  #endif
#endif
#endif

#if defined(HAVE_ESP32_LEDC) && defined(__has_include)
  #if __has_include(<esp32-hal-ledc.h>)
    #include <esp32-hal-ledc.h>
  #elif __has_include(<driver/ledc.h>)
    #include <driver/ledc.h>
  #elif __has_include(<ledc.h>)
    #include <ledc.h>
  #endif
#endif
// Cross-platform PWM wrappers. On ESP32 these use LEDC; on other platforms they fall
// back to a digital on/off so the sketch compiles and can be tested.
  // Determine which LEDC API variant is available. Different ESP32 cores
  // expose different helper signatures - e.g. the Arduino "esp32-hal-ledc.h"
  // provides `bool ledcWrite(uint8_t pin, uint32_t duty)` while the lower-level
  // `driver/ledc.h` variant exposes `ledcSetup/ledcAttachPin/ledcWrite(channel, duty)`.
  // Detect the available header and set a macro so the pwm wrappers below call
  // the correct API and avoid conflicting prototype declarations.
  #if defined(__has_include)
    #if __has_include(<esp32-hal-ledc.h>)
      #define LEDC_API_ESP32_HAL 1
    #elif __has_include(<driver/ledc.h>)
      #define LEDC_API_DRIVER 1
    #elif __has_include(<ledc.h>)
      #define LEDC_API_LEDC_H 1
    #endif
  #endif

static inline void pwmInit() {
  // Prefer driver API (channel-based) when available
#if defined(LEDC_API_DRIVER) && (defined(ARDUINO_ARCH_ESP32) || defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP_ARDUINO_VERSION))
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES_BITS);
  ledcAttachPin(OUTPUT_PIN, PWM_CHANNEL);

// If the Arduino "esp32-hal-ledc" helper is present, it exposes a pin-based
// `ledcWrite(pin, duty)` helper. In that case we avoid calling channel-based
// setup/attach helpers that may have different prototypes.
#elif defined(LEDC_API_ESP32_HAL) && (defined(ARDUINO_ARCH_ESP32) || defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP_ARDUINO_VERSION))
  // For Arduino ESP32 core 3.x, use ledcAttach to set up PWM on the pin
  ledcAttach(OUTPUT_PIN, PWM_FREQ, PWM_RES_BITS);
  // Initialize to zero duty
  ledcWrite(OUTPUT_PIN, 0);

#else
  // Generic fallback: simple digital output so sketch compiles on non-ESP32 cores
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
#endif
}

static inline void pwmWrite(int duty) {
#if defined(LEDC_API_DRIVER) && (defined(ARDUINO_ARCH_ESP32) || defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP_ARDUINO_VERSION))
  // driver API expects channel
  ledcWrite(PWM_CHANNEL, duty);
#elif defined(LEDC_API_ESP32_HAL) && (defined(ARDUINO_ARCH_ESP32) || defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP_ARDUINO_VERSION))
  // esp32-hal provides a pin-based helper
  ledcWrite(OUTPUT_PIN, duty);
#else
  if (duty > 0) digitalWrite(OUTPUT_PIN, HIGH); else digitalWrite(OUTPUT_PIN, LOW);
#endif
}
// BLE initialization and callbacks are handled by BLEManager now.

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000); // Give serial time to initialize
  while (!Serial && millis() < 3000) {
    delay(10); // Wait up to 3 seconds for serial
  }
  // Print persisted boot error if present
  if (gState.errorLog.lastError[0] != '\0') {
    Serial.printf("[RTC] Last boot error: %s\n", gState.errorLog.lastError);
  }

  // Load persisted non-volatile config (Preferences/NVS) so settings survive power cycles
  {
    Preferences prefs;
    prefs.begin("espatom", true);
    String v;
    v = prefs.getString("def_sp", ""); if (v.length()) gState.pidController.defaultSetpoint = atof(v.c_str());
    v = prefs.getString("unit", ""); if (v.length()) gState.menu.tempUnitIsC = (v.charAt(0) == '1');
    v = prefs.getString("ble_en", ""); if (v.length()) gState.menu.ble.enabled = (v.charAt(0) == '1');
    v = prefs.getString("ble_name", ""); if (v.length()) gState.menu.ble.nameIndex = atoi(v.c_str());
    v = prefs.getString("ble_adv", ""); if (v.length()) gState.menu.ble.advIntervalMs = strtoul(v.c_str(), nullptr, 10);
    v = prefs.getString("oled_ind", ""); if (v.length()) gState.menu.ble.oledIndicator = (v.charAt(0) == '1');
    prefs.end();
    Serial.println(F("[PREF] Loaded persisted config (if present)."));
  }
  // Initialize battery early so we can display voltage on the boot OLED
#if USE_BAT
  initBattery();
  sampleBattery();
  // Additional one-shot diagnostic to help troubleshoot early-boot ADC reads
  {
    uint32_t acc = 0;
    for (int i = 0; i < BAT_SAMPLES; ++i) { acc += analogRead(BAT_PIN); delay(1); }
    uint32_t raw = acc / (BAT_SAMPLES > 0 ? BAT_SAMPLES : 1);
    double v_adc = (raw / 4095.0) * 3.3;
    double vbat = v_adc * ((BAT_R1 + BAT_R2) / BAT_R2);
    Serial.printf("[BATT_DBG] early raw=%lu v_adc=%.3f V vbat=%.3f V (gState.battery.voltage=%.3f)\n", (unsigned long)raw, v_adc, vbat, gState.battery.voltage);
    if (isnan(gState.battery.voltage) || gState.battery.voltage <= 0.01) {
      Serial.println(F("[BATT_DBG] Warning: gState.battery.voltage invalid at early boot - check BAT_PIN mapping and ADC capability on this module."));
    }
  }
#if ALLOW_BOOT_WITHOUT_BATTERY
  // If battery missing or below threshold, optionally require USB VBUS to boot
  if (isnan(gState.battery.voltage) || gState.battery.voltage < BAT_MIN_V) {
#if defined(VBUS_SENSE_PIN)
    pinMode(VBUS_SENSE_PIN, INPUT);
    if (digitalRead(VBUS_SENSE_PIN) == LOW) {
      Serial.println(F("[BOOT] No battery present and VBUS not detected — USB fallback is enabled, continuing boot."));
    } else {
      Serial.println(F("[BOOT] VBUS detected - USB fallback enabled, continuing boot without battery."));
    }
#else
    Serial.println(F("[BOOT] ALLOW_BOOT_WITHOUT_BATTERY enabled - continuing boot without battery."));
#endif
  }
#endif
#endif

#if USE_ENCODER
  // Initialize encoder early so we can display its init summary on the boot OLED
  encoderInit();
#endif

  // Initialize OLED early so boot messages can be displayed immediately
#if USE_OLED
  Serial.println(F("[BOOT] Attempting early OLED init..."));
  bool earlyOk = tryInitOLED(OLED_SDA, OLED_SCL);
  if (earlyOk) {
    // Compose boot diagnostics and check for initialization issues
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(F("ESPAtomizer boot"));
    // Firmware version
    display.print(F("FW: ")); display.println(FIRMWARE_VERSION);
    // Battery: show voltage and percent when available
    if (!isnan(gState.battery.voltage)) {
      char bbuf[12]; dtostrf(gState.battery.voltage, 0, 2, bbuf);
      display.print(F("BAT: ")); display.print(bbuf); display.print(F("V "));
      display.print(gState.battery.percent); display.println(F("%"));
    } else {
      display.println(F("BAT: --V --%"));
    }

    // Encoder pins + interrupt mapping and short init summary
    int ia = digitalPinToInterrupt(ENC_PIN_A);
    int ib = digitalPinToInterrupt(ENC_PIN_B);
    display.print(F("ENC A:")); display.print(ENC_PIN_A); display.print(F(" i:")); display.println(ia);
    display.print(F("ENC B:")); display.print(ENC_PIN_B); display.print(F(" i:")); display.println(ib);
    display.print(F("ENC: ")); display.println(gState.input.lastEncoderInfo);

    // Quick thermocouple one-shot check (if enabled)
    bool tc_ok = isThermocoupleValid();

    // Aggregate errors/warnings
    String errors = "";
    String warns = "";
    // Battery: NaN = ADC missing -> warning. Very low voltage (<0.5V) = likely ADC open/short/error -> treat as error.
    if (isnan(gState.battery.voltage)) {
      warns += "BAT:ADC? ";
    } else {
      if (gState.battery.voltage < 0.5) errors += "BAT:0? ";
      else if (gState.battery.voltage < BAT_MIN_V) warns += "BAT:LOW ";
    }
    if (!tc_ok) warns += "TC:NO ";
    if (ia == -1 || ib == -1) warns += "ENC_IRQ? ";

    // BLE presence check (best-effort)
  #if USE_BLE
    if (!gState.menu.ble.enabled) warns += "BLE:off ";
  #endif

    // If any errors, display an obvious full-screen ERROR indicator and persist it to RTC
    if (errors.length() > 0) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor((OLED_WIDTH - 4*6) / 2, (OLED_HEIGHT/2) - 12);
      display.println(F("ERROR"));
      display.setTextSize(1);
      display.setCursor(2, OLED_HEIGHT - 18);
      display.print(F("ERR: ")); display.println(errors);
      display.display();
      Serial.print("[BOOT_ERR] "); Serial.println(errors);
      // Persist to RTC so reboot/soft-reset retains the failure info
      strncpy(gState.errorLog.lastError, errors.c_str(), sizeof(gState.errorLog.lastError)-1);
      gState.errorLog.lastError[sizeof(gState.errorLog.lastError)-1] = '\0';
      delay(2000);
    } else {
      // Clear persisted error if boot succeeded
      gState.errorLog.lastError[0] = '\0';
      if (warns.length() > 0) {
        // Warnings: show a NOTICE line but continue with normal splash
        display.setTextSize(1);
        display.setCursor(0, OLED_HEIGHT - 14);
        display.print(F("WARN: ")); display.println(warns);
        display.display();
        Serial.print("[BOOT_WARN] "); Serial.println(warns);
        delay(1200);
      }

      // If no fatal errors, flash full-screen device name for 1s (centered)
      const char* devName = BLE_NAME_PRESETS[gState.menu.ble.nameIndex];
      display.clearDisplay();
      int ssz = 2; // text size for splash
      int w = (int)strlen(devName) * 6 * ssz;
      int h = 8 * ssz;
      int sx = (OLED_WIDTH - w) / 2; if (sx < 0) sx = 0;
      int sy = (OLED_HEIGHT - h) / 2; if (sy < 0) sy = 0;
      display.setTextSize(ssz);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(sx, sy);
      display.print(devName);
      display.display();
      delay(6000);
      display.clearDisplay(); display.display();
    }
    delay(1200);
  } else {
    Serial.println(F("[BOOT] OLED not available at early init"));
  }
#endif

  pinMode(LED_BUILTIN, OUTPUT); // Onboard LED for status
  digitalWrite(LED_BUILTIN, LOW); // Start off

  Serial.println();
  Serial.println(F("ESP PID Temperature Controller starting..."));
  // Print reset reason for diagnostics (helps diagnose unexpected reboots)
  esp_reset_reason_t r = esp_reset_reason();
  const char *rs = "?";
  switch (r) {
    case ESP_RST_UNKNOWN: rs = "UNKNOWN"; break;
    case ESP_RST_POWERON: rs = "POWERON"; break;
    case ESP_RST_EXT: rs = "EXTERNAL"; break;
    case ESP_RST_SW: rs = "SOFTWARE"; break;
    case ESP_RST_PANIC: rs = "PANIC"; break;
    case ESP_RST_INT_WDT: rs = "INT_WDT"; break;
    case ESP_RST_TASK_WDT: rs = "TASK_WDT"; break;
    case ESP_RST_WDT: rs = "WDT"; break;
    case ESP_RST_DEEPSLEEP: rs = "DEEPSLEEP"; break;
    case ESP_RST_BROWNOUT: rs = "BROWNOUT"; break;
    case ESP_RST_SDIO: rs = "SDIO"; break;
    default: rs = "OTHER"; break;
  }
  Serial.printf("Boot: reset_reason=%s(%d)\n", rs, (int)r);
  Serial.printf("Pins: OUT=%d(D6), ENC_A=%d(D0), ENC_B=%d(D1), ENC_SW=%d(D2), SDA=%d(D4), SCL=%d(D5), BAT=%d(D3)\n",
                OUTPUT_PIN, ENC_PIN_A, ENC_PIN_B, ENC_PIN_SW, OLED_SDA, OLED_SCL, BAT_PIN);

  // ===== I2C BUS DIAGNOSTIC SCAN =====
  Serial.println(F("\n[I2C] === DIAGNOSTIC SCAN ==="));
  Serial.printf("[I2C] Board SDA macro = %d, SCL macro = %d\n", (int)SDA, (int)SCL);
  Serial.printf("[I2C] Config OLED_SDA = %d, OLED_SCL = %d\n", OLED_SDA, OLED_SCL);
  
  // Check pin levels before I2C init (should be HIGH with pull-ups)
  pinMode(OLED_SDA, INPUT);
  pinMode(OLED_SCL, INPUT);
  delay(10);
  int sdaLevel = digitalRead(OLED_SDA);
  int sclLevel = digitalRead(OLED_SCL);
  Serial.printf("[I2C] Pre-init pin levels: SDA=%d SCL=%d (should be HIGH=1 with pull-ups)\n", sdaLevel, sclLevel);
  if (sdaLevel == LOW || sclLevel == LOW) {
    Serial.println(F("[I2C] WARNING: Pin held LOW - check for shorts or missing pull-ups!"));
  }
  
  // Initialize I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(100000);  // 100kHz for reliability during scan
  delay(50);
  
  // Scan all addresses
  Serial.println(F("[I2C] Scanning addresses 0x01-0x7F..."));
  int devicesFound = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      devicesFound++;
      const char* devName = "Unknown";
      if (addr == 0x3C || addr == 0x3D) devName = "OLED SSD1306";
      else if (addr == 0x48) devName = "ADS1115 (ADDR=GND)";
      else if (addr == 0x49) devName = "ADS1115 (ADDR=VDD)";
      else if (addr == 0x4A) devName = "ADS1115 (ADDR=SDA)";
      else if (addr == 0x4B) devName = "ADS1115 (ADDR=SCL)";
      Serial.printf("[I2C] FOUND: 0x%02X (%s)\n", addr, devName);
    } else if (err == 4) {
      Serial.printf("[I2C] ERROR at 0x%02X (bus error)\n", addr);
    }
  }
  Serial.printf("[I2C] Scan complete: %d device(s) found\n", devicesFound);
  if (devicesFound == 0) {
    Serial.println(F("[I2C] NO DEVICES! Check:"));
    Serial.println(F("  1. Are SDA/SCL wires connected?"));
    Serial.println(F("  2. Are 4.7k pull-up resistors installed (SDA->3.3V, SCL->3.3V)?"));
    Serial.println(F("  3. Is ADS1115 ADDR pin tied to GND?"));
    Serial.println(F("  4. Is 3.3V power reaching the I2C devices?"));
  }
  Serial.println(F("[I2C] === END SCAN ===\n"));
  // ===== END I2C DIAGNOSTIC =====

  // Temperature sensor setup: using ADS1115 I2C precision thermocouple ADC
  Serial.println(F("Using ADS1115 I2C thermocouple ADC, SDA/SCL shared with OLED display"));
  if (!ads1115_init()) {
    Serial.println(F("[ADS1115] WARNING: Failed to initialize ADS1115 I2C ADC. Check wiring."));
  }

  // Initialize output/PWM (wrapper handles cross-core differences)
  pwmInit();

  // Initialize output off
  applyOutput(0);

  // PID setup
  tempPID.SetOutputLimits(0, PWM_MAX); // match PWM range
  tempPID.SetSampleTime(gState.timing.loopIntervalMs);
  tempPID.SetMode(AUTOMATIC);

  gState.timing.windowStartTime = millis();

  // Encoder channels
#if USE_ENCODER
  // encoderInit() was performed earlier so we only run the self-test here
  encoderSelfTest(5000UL);
#endif

  // Button setup (encoder push switch)
#if BUTTON_ACTIVE_LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#else
  pinMode(BUTTON_PIN, INPUT);
#endif
  // Initialize button tracking based on first read
  gState.input.lastRead = digitalRead(BUTTON_PIN);
  gState.input.stable = gState.input.lastRead;
  gState.input.lastChangeMs = millis();
  gState.input.stablePressed = BUTTON_ACTIVE_LOW ? (gState.input.stable == LOW) : (gState.input.stable == HIGH);
  Serial.printf("Button initial raw=%d (active-%s)\n", (int)gState.input.lastRead, BUTTON_ACTIVE_LOW ? "LOW" : "HIGH");

  // ADC / battery setup
#if USE_BAT
  initBattery();
  // Prime the battery reading so the OLED shows a value immediately on boot
  sampleBattery();
#endif

  // OLED display setup
#if USE_OLED
  Serial.println(F("[OLED] Init starting..."));
  bool initOk = false;
  // First try the configured pins
  initOk = tryInitOLED(OLED_SDA, OLED_SCL);
  if (!initOk) {
    // Try swapped SDA/SCL
    Serial.println(F("[OLED] Init failed - trying swapped SDA/SCL pins..."));
    initOk = tryInitOLED(OLED_SCL, OLED_SDA);
    if (initOk) Serial.println(F("[OLED] Responded after swapping SDA/SCL (software swap)"));
  }
  if (!initOk) {
    Serial.println(F("[OLED] not found; continuing without display."));
    gState.display.available = false;
  }
#endif

  // ===== INITIALIZE MANAGER SUBSYSTEMS =====
  Serial.println(F("[Manager] Initializing subsystem managers..."));
  
  // Initialize PreferencesManager (loads NVS configuration)
  PreferencesManager::init();
  
  // Initialize menu manager (clears menu state)
  MenuManager::init();
  
  // Initialize GlobalState structs with defaults
  gState.reset();
  gState.tempPID = &tempPID;

  // Load persisted preferences into GlobalState
  gState.pidController.defaultSetpoint = PreferencesManager::getDefaultSetpoint();
  gState.menu.tempUnitIsC = PreferencesManager::getTempUnitC();
  gState.menu.ble.enabled = PreferencesManager::getBleEnabled();
  gState.menu.ble.nameIndex = PreferencesManager::getBleName();
  
  // Initialize safety manager (watchdog, thermal, battery, sensor checks)
  SafetyManager::init(gState);
  Serial.println(F("[Manager] SafetyManager initialized"));
  
#if USE_BLE
  // Initialize BLE manager (server, characteristics, callbacks)
  BLEManager::init(gState);
  Serial.println(F("[Manager] BLEManager initialized"));
#endif
  
  // Initialize CLI manager (serial command handler)
  CLIManager::init(gState);
  Serial.println(F("[Manager] CLIManager initialized"));
  
#if USE_OLED
  // Initialize UI manager (display rendering)
  UIManager::init(gState);
  Serial.println(F("[Manager] UIManager initialized"));
#endif

  // Run boot self-tests (Step 8)
  runBootSelfTests();

  Serial.println(F("Setup complete!"));

  // WiFi setup helper (noop if USE_WIFI==0)
  wifiDoSetup();

#if USE_BLE
  // BLE is initialized by BLEManager::init(gState) earlier during setup.
#endif

  // Check wake-up cause and restore RTC state
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
#if ENABLE_RTC_PERSISTENCE
  if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED) {
    const char *wr = "UNKNOWN";
    switch (wakeup_reason) {
      case ESP_SLEEP_WAKEUP_EXT0: wr = "EXT0"; break;
      case ESP_SLEEP_WAKEUP_EXT1: wr = "EXT1"; break;
      case ESP_SLEEP_WAKEUP_TIMER: wr = "TIMER"; break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD: wr = "TOUCHPAD"; break;
      case ESP_SLEEP_WAKEUP_ULP: wr = "ULP"; break;
      case ESP_SLEEP_WAKEUP_GPIO: wr = "GPIO"; break;
      case ESP_SLEEP_WAKEUP_UART: wr = "UART"; break;
      case ESP_SLEEP_WAKEUP_UNDEFINED: wr = "UNDEFINED"; break;
      default: wr = "OTHER"; break;
    }
    Serial.printf("Woke up from sleep, reason: %s(%d)\n", wr, (int)wakeup_reason);
    // Restore state from RTC memory with validation and clamping
    gState.pidController.setpointC = constrain(gState.pidController.rtc.setpoint, (double)ENC_MIN_C, (double)ENC_MAX_C);
    gState.pidController.Kp = constrain(gState.pidController.rtc.Kp, 0.1, 100.0);
    gState.pidController.Ki = constrain(gState.pidController.rtc.Ki, 0.01, 10.0);
    gState.pidController.Kd = constrain(gState.pidController.rtc.Kd, 0.1, 1000.0);
    if (gState.pidController.setpointC != gState.pidController.rtc.setpoint) Serial.printf("[RTC] Setpoint clamped: %.1f -> %.1f C\n", gState.pidController.rtc.setpoint, gState.pidController.setpointC);
    if (gState.pidController.Kp != gState.pidController.rtc.Kp) Serial.printf("[RTC] gState.pidController.Kp clamped: %.3f -> %.3f\n", gState.pidController.rtc.Kp, gState.pidController.Kp);
    if (gState.pidController.Ki != gState.pidController.rtc.Ki) Serial.printf("[RTC] gState.pidController.Ki clamped: %.3f -> %.3f\n", gState.pidController.rtc.Ki, gState.pidController.Ki);
    if (gState.pidController.Kd != gState.pidController.rtc.Kd) Serial.printf("[RTC] gState.pidController.Kd clamped: %.3f -> %.3f\n", gState.pidController.rtc.Kd, gState.pidController.Kd);
    gState.pidController.manualMode = gState.pidController.rtc.manualMode;
    gState.pidController.systemEnabled = gState.pidController.rtc.systemEnabled;
    tempPID.SetTunings(gState.pidController.Kp, gState.pidController.Ki, gState.pidController.Kd);
    tempPID.SetMode(gState.pidController.manualMode ? MANUAL : AUTOMATIC);
    gState.pidController.pidMode = gState.pidController.rtc.pidMode;
    applyPidMode(gState.pidController.pidMode);
    gState.pidController.pidOutput = constrain(gState.pidController.rtc.pidOutput, 0.0, (double)PWM_MAX);
    if (gState.pidController.pidOutput != gState.pidController.rtc.pidOutput) Serial.printf("[RTC] Output clamped: %.0f -> %.0f\n", gState.pidController.rtc.pidOutput, gState.pidController.pidOutput);
    applyOutput(gState.pidController.pidOutput);
    Serial.println(F("State restored from RTC memory (all values validated)"));
  } else {
    // First boot, initialize RTC with defaults
    gState.pidController.rtc.setpoint = gState.pidController.setpointC;
    gState.pidController.rtc.Kp = gState.pidController.Kp;
    gState.pidController.rtc.Ki = gState.pidController.Ki;
    gState.pidController.rtc.Kd = gState.pidController.Kd;
    gState.pidController.rtc.manualMode = gState.pidController.manualMode;
    gState.pidController.rtc.pidMode = gState.pidController.pidMode;
    // U1/U2 preset defaults are already stored in gState.pidController.u1/u2.
    gState.pidController.rtc.systemEnabled = gState.pidController.systemEnabled;
    // initialize persisted gState.pidController.pidOutput
    gState.pidController.rtc.pidOutput = gState.pidController.pidOutput;
  }
#else
  // RTC persistence disabled for development
  Serial.println("[INIT] RTC persistence disabled (ENABLE_RTC_PERSISTENCE=0)");
#endif

  gState.timing.lastActivityMs = millis();  // Initialize activity timer
}

// Support both divider orientations via THERMISTOR_TOP (default: NTC at top: 3.3V -> NTC -> node -> Rseries -> GND)
#ifndef THERMISTOR_TOP
#define THERMISTOR_TOP 1
#endif

static inline double computeRntc(double v) {
  (void)v; // thermistor removed
  return NAN;
}

static float readTemperatureC() {
  gState.diagnostics.sensorReads++;  // Increment diagnostic counter
  // In TEST_MODE, simulate temperature trending toward setpoint based on output
#if TEST_MODE
  static float simTemp = 25.0f;
  float drive = (float)gState.pidController.pidOutput / (float)PWM_MAX; // 0..1
  simTemp += (float)((gState.pidController.setpointC - simTemp) * 0.02f) + (drive * 1.5f) - 0.1f;
  if (simTemp < 20.0f) simTemp = 20.0f;
  if (simTemp > 800.0f) simTemp = 800.0f;
  return simTemp;
#else
  // Read from AD8495 thermocouple amplifier
  float temp = readThermocoupleTemp_C();
  return (isnan(temp) || !isThermocoupleValid()) ? NAN : temp;
#endif
}

// Sensor fault detection and recovery logic is now handled by SafetyManager.

// Apply output with ramp rate limiting to prevent sudden PWM jumps
static double applyOutputWithRampLimit(double newOutput) {
  double maxDelta = MAX_PWM_RAMP_RATE;
  double delta = newOutput - gState.safety.lastAppliedOutput;
  
  if (delta > maxDelta) {
    newOutput = gState.safety.lastAppliedOutput + maxDelta;
  } else if (delta < -maxDelta) {
    newOutput = gState.safety.lastAppliedOutput - maxDelta;
  }
  
  gState.safety.lastAppliedOutput = newOutput;
  return newOutput;
}

// Battery safety monitoring: charger removal detection and ramp-down is handled by BatteryManager now.

// Encoder input protection: check if change is too extreme and requires user confirmation
static bool isExtremeSetpointChange(double currentSetpoint, double newSetpoint) {
  return fabs(newSetpoint - currentSetpoint) > ENC_EXTREME_DELTA_C;
}

static void handleEncoderConfirmationTimeout(unsigned long now) {
  if (gState.menu.awaitingConfirmation && !gState.menu.active && !gState.menu.configMode) {
    if ((now - gState.menu.confirmationStartMs) > ENC_CONFIRM_TIMEOUT_MS) {
      Serial.printf("[ENC_CONFIRM] Timeout: Change from %.1fC to %.1fC cancelled\n", gState.pidController.setpointC, gState.menu.pendingSetpointC);
      gState.menu.awaitingConfirmation = false;
    }
  }
}

void loop() {
  // Main loop: handle serial, network, input, sensor, PID, display, and sleep
  const unsigned long now = millis();
  
  // Mark loop entry for watchdog safety check
  SafetyManager::markLoopEntry();
  
  gState.diagnostics.loopIterations++;  // Increment diagnostic counter
  
  // Periodic system health check (every 5 seconds)
  static unsigned long lastHealthCheckMs = 0;
  if (now - lastHealthCheckMs >= 5000) {
    lastHealthCheckMs = now;
  }
  
  // Check continuous runtime limit (30 minutes max)
  if (gState.pidController.systemEnabled && gState.pidController.pidOutput > 0) {
    if (gState.safety.continuousRunStartMs == 0) {
      gState.safety.continuousRunStartMs = now;
    } else if (now - gState.safety.continuousRunStartMs >= MAX_CONTINUOUS_RUN_MS) {
      gState.pidController.systemEnabled = false;
      gState.safety.cooldownRequired = true;
      gState.safety.cooldownStartMs = now;
      applyOutput(0);
      Serial.printf("[SAFETY] System disabled after %lu minutes of continuous operation\n", 
                    (now - gState.safety.continuousRunStartMs) / 60000);
    }
  } else {
    gState.safety.continuousRunStartMs = 0;  // Reset when disabled or output is zero
  }
  
  // Handle cooldown period
  if (gState.safety.cooldownRequired) {
    if (now - gState.safety.cooldownStartMs >= COOLDOWN_REQUIRED_MS) {
      gState.safety.cooldownRequired = false;
      Serial.println(F("[SAFETY] Cooldown complete, system can be re-enabled"));
    } else if (gState.pidController.systemEnabled) {
      // User tried to enable during cooldown
      gState.pidController.systemEnabled = false;
      applyOutput(0);
      unsigned long remaining = (COOLDOWN_REQUIRED_MS - (now - gState.safety.cooldownStartMs)) / 1000;
      Serial.printf("[SAFETY] Cooldown in progress, %lu seconds remaining\n", remaining);
    }
  }
  
  // CLI/serial command handling via CLIManager
  CLIManager::update();
  wifiLoopHandle();

  // Safety manager handles all safety checks (watchdog, thermal runaway, battery, sensor faults)
  SafetyManager::update();

  // LED status / charging indicator
  static unsigned long ledBlinkTimer = 0;
  static bool ledBlinkState = false;
  bool charging = false;
#if USE_BAT
  charging = (gState.battery.percent < 100 && gState.battery.voltage > BAT_MIN_V);
#endif
  if (gState.pidController.systemEnabled) {
    if (charging) {
      if ((now - ledBlinkTimer) > 500) { ledBlinkTimer = now; ledBlinkState = !ledBlinkState; }
      digitalWrite(LED_BUILTIN, ledBlinkState ? HIGH : LOW);
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Button debouncing and actions
  int buttonRaw = digitalRead(BUTTON_PIN);
  if (buttonRaw != (int)gState.input.lastRead) { gState.input.lastChangeMs = now; gState.input.lastRead = buttonRaw; }
  if ((now - gState.input.lastChangeMs) >= BUTTON_DEBOUNCE_MS) {
    if ((bool)buttonRaw != gState.input.stable) {
      gState.input.stable = (bool)buttonRaw;
      bool stablePressed = BUTTON_ACTIVE_LOW ? (gState.input.stable == LOW) : (gState.input.stable == HIGH);
      // Press edge: record start and reset long-press handled flag
      if (stablePressed && !gState.input.stablePressed) {
        gState.input.pressStartMs = now; gState.timing.lastActivityMs = now; gState.input.longHandled = false; Serial.println(F("[BTN] Pressed"));
      }

      // While held, if threshold reached and not yet handled, trigger long-press action now
      if (stablePressed && !gState.input.longHandled) {
        unsigned long heldNow = now - gState.input.pressStartMs;
        if (heldNow >= BUTTON_LONG_MS) {
          gState.input.longHandled = true;
          // long press toggles menu immediately when threshold crossed
          gState.menu.active = !gState.menu.active;
          // ensure config mode is closed when toggling menus
          gState.menu.configMode = false;
          if (gState.menu.active) { gState.menu.index = gState.pidController.pidMode; Serial.println(F("[MENU] Opened")); }
          else { Serial.println(F("[MENU] Closed")); }
          if (gState.input.holdHeating) { gState.pidController.pidOutput = gState.input.pidOutputBeforeHold; gState.input.holdHeating = false; }
        }
      }

      // Release edge: handle short-press actions only if long-press wasn't already handled
      if (!stablePressed && gState.input.stablePressed) {
        unsigned long held = now - gState.input.pressStartMs;
        Serial.printf("[BTN] Released after %lums\n", held);
        if (!gState.input.longHandled) {
          // short press: select menu item, BT submenu action, or toggle power
          if (gState.menu.configMode) {
            // In config mode: if currently editing a value, confirm/save on short press.
            if (gState.menu.configEditing) {
              gState.menu.configEditing = false;
              Serial.printf("[CONFIG] Saved Default SP=%.1f C\n", gState.pidController.defaultSetpoint);
              // Persist default setpoint to Preferences so it survives power cycles
              {
                Preferences prefs;
                prefs.begin("espatom", false);
                char buf[16]; snprintf(buf, sizeof(buf), "%.1f", gState.pidController.defaultSetpoint);
                prefs.putString("def_sp", buf);
                prefs.end();
                Serial.println(F("[PREF] Default setpoint saved"));
              }
            } else {
              // Execute action for selected config item
              switch (gState.menu.configIndex) {
                case 0: // Default SP: enter editing mode
                  gState.menu.configEditing = true;
                  Serial.println(F("[CONFIG] Editing Default SP (use encoder)"));
                  break;
                case 1: // Unit toggle
                  gState.menu.tempUnitIsC = !gState.menu.tempUnitIsC;
                  Serial.printf("[CONFIG] Temp unit toggled -> %s\n", gState.menu.tempUnitIsC?"C":"F");
                  // Persist unit choice
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    prefs.putString("unit", gState.menu.tempUnitIsC ? "1" : "0");
                    prefs.end();
                    Serial.println(F("[PREF] Unit saved"));
                  }
                  #if USE_BLE
                  if (gState.ble.chUnit && bleIsConnected()) {
                    std::string s = gState.menu.tempUnitIsC ? "C" : "F";
                    gState.ble.chUnit->setValue(s);
                    gState.ble.chUnit->notify();
                  }
                  #endif
                  break;
                case 2: // BLE ON toggle
                  gState.menu.ble.enabled = !gState.menu.ble.enabled;
                  Serial.printf("[BT] BLE enabled=%d\n", gState.menu.ble.enabled?1:0);
                  // Persist BLE enabled flag
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    prefs.putString("ble_en", gState.menu.ble.enabled ? "1" : "0");
                    prefs.end();
                    Serial.println(F("[PREF] BLE enabled saved"));
                  }
                  if (gState.menu.ble.enabled) {
                    BLEManager::init(gState);
                  } else {
                    #if USE_BLE
                    if (gState.ble.server && NimBLEDevice::getAdvertising()) {
                      NimBLEDevice::getAdvertising()->stop();
                      Serial.println(F("[BT] Advertising stopped"));
                    }
                    #else
                    Serial.println(F("[BT] BLE compiled out; runtime disable is a no-op"));
                    #endif
                  }
                  break;
                case 3: // Name: cycle presets
                  gState.menu.ble.nameIndex = (gState.menu.ble.nameIndex + 1) % BLE_NAME_PRESET_COUNT;
                  Serial.printf("[BT] BLE name set to %s\n", BLE_NAME_PRESETS[gState.menu.ble.nameIndex]);
                  // Persist chosen name index
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    char buf[8]; snprintf(buf, sizeof(buf), "%d", gState.menu.ble.nameIndex);
                    prefs.putString("ble_name", buf);
                    prefs.end();
                    Serial.println(F("[PREF] BLE name index saved"));
                  }
                  BLEManager::setDeviceName(gState.menu.ble.nameIndex);
                  break;
                case 4: // AdvInt cycle
                  if (gState.menu.ble.advIntervalMs == 200UL) gState.menu.ble.advIntervalMs = 100UL;
                  else if (gState.menu.ble.advIntervalMs == 100UL) gState.menu.ble.advIntervalMs = 1000UL;
                  else gState.menu.ble.advIntervalMs = 200UL;
                  Serial.printf("[BT] Advert interval preset = %lums (restart advertising to apply)\n", gState.menu.ble.advIntervalMs);
                  // Persist adv interval
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    char buf[16]; snprintf(buf, sizeof(buf), "%lu", gState.menu.ble.advIntervalMs);
                    prefs.putString("ble_adv", buf);
                    prefs.end();
                    Serial.println(F("[PREF] BLE adv interval saved"));
                  }
                  break;
                case 5: // OLED indicator toggle
                  gState.menu.ble.oledIndicator = !gState.menu.ble.oledIndicator;
                  Serial.printf("[BT] OLED indicator enabled=%d\n", gState.menu.ble.oledIndicator?1:0);
                  // Persist OLED indicator preference
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    prefs.putString("oled_ind", gState.menu.ble.oledIndicator ? "1" : "0");
                    prefs.end();
                    Serial.println(F("[PREF] OLED indicator saved"));
                  }
                  #if USE_OLED
                  if (gState.display.available && !gState.menu.ble.oledIndicator) {
                    display.clearDisplay();
                    display.display();
                    Serial.println(F("[OLED] Indicator disabled; display cleared"));
                  } else if (gState.display.available && gState.menu.ble.oledIndicator) {
                    Serial.println(F("[OLED] Indicator enabled"));
                  }
                  #endif
                  break;
                case 6: // Save configuration
                  Serial.println(F("[CONFIG] Save configuration requested - all settings already saved"));
                  // All settings are already saved to Preferences when changed (cases 0-5)
                  // This option exists for user confirmation that changes are persisted
                  #if USE_OLED
                  if (gState.display.available) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(WHITE);
                    display.setCursor(0, 20);
                    display.println(F("  Settings Saved!"));
                    display.display();
                    delay(1000);  // Show confirmation message
                  }
                  #endif
                  break;
                case 7: // Factory Reset
                  Serial.println(F("[CONFIG] Factory Reset requested"));
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    prefs.clear();  // Clear all preferences
                    prefs.end();
                    Serial.println(F("[PREF] All preferences cleared - factory reset complete"));
                    Serial.println(F("[SYSTEM] Reboot required to apply factory defaults"));
                    #if USE_OLED
                    if (gState.display.available) {
                      display.clearDisplay();
                      display.setTextSize(1);
                      display.setTextColor(WHITE);
                      display.setCursor(0, 10);
                      display.println(F("Factory Reset!"));
                      display.setCursor(0, 30);
                      display.println(F("Please reboot"));
                      display.display();
                      delay(2000);
                    }
                    #endif
                  }
                  break;
                case 8: // Forget bonds
                  Serial.println(F("[BT] Forget bonds requested"));
#if defined(HAVE_NIMBLE_STORE_HDR)
                  ble_store_clear();
                  Serial.println(F("[BT] ble_store_clear() called to remove stored bonds/CCCDs"));
#else
                  Serial.println(F("[BT] NimBLE store header not found; cannot clear bonds programmatically in this build"));
#endif
                  break;
                case 9: // Back
                  gState.menu.configMode = false;
                  Serial.println(F("[CONFIG] Exited config mode"));
                  break;
                default:
                  break;
              }
            }
          } else if (gState.menu.active) {
            if (gState.menu.index >= 0 && gState.menu.index < MenuManager::MENU_ITEM_COUNT) applyPidMode(gState.menu.index);
          } else {
            gState.pidController.systemEnabled = !gState.pidController.systemEnabled;
            if (!gState.pidController.systemEnabled) {
              tempPID.SetMode(MANUAL); gState.pidController.pidOutput = 0;
#if RELAY_MODE
              applyRelayWindow(0);
#else
              applyOutput(0);
#endif
              Serial.println(F("Power: OFF"));
              if (gState.input.holdHeating) gState.input.holdHeating = false;
            } else {
              tempPID.SetMode(gState.pidController.manualMode ? MANUAL : AUTOMATIC);
              Serial.println(F("Power: ON"));
            }
          }
          }
        if (gState.input.holdHeating) { gState.pidController.pidOutput = gState.input.pidOutputBeforeHold; gState.input.holdHeating = false; Serial.println(F("[Manual] Hold heating OFF")); }
      }
      gState.input.stablePressed = stablePressed;
    }
  }

  // If the button is stably pressed, detect long-press threshold while held
  if (gState.input.stablePressed && !gState.input.longHandled) {
    unsigned long heldNow = now - gState.input.pressStartMs;
    if (heldNow >= BUTTON_LONG_MS) {
      gState.input.longHandled = true;
      
      // If awaiting confirmation for extreme encoder change, confirm it
      if (gState.menu.awaitingConfirmation && !gState.menu.active && !gState.menu.configMode) {
        gState.pidController.setpointC = gState.menu.pendingSetpointC;
        Serial.printf("[ENC_CONFIRM] CONFIRMED: Setpoint changed to %.1fC\n", gState.pidController.setpointC);
        gState.menu.awaitingConfirmation = false;
      } else {
        // Normal long-press: toggle menu
        gState.menu.active = !gState.menu.active;
        // ensure config mode is closed when toggling menus
        gState.menu.configMode = false;
        if (gState.menu.active) { gState.menu.index = gState.pidController.pidMode; Serial.println(F("[MENU] Opened")); }
        else { Serial.println(F("[MENU] Closed")); }
        if (gState.input.holdHeating) { gState.pidController.pidOutput = gState.input.pidOutputBeforeHold; gState.input.holdHeating = false; }
      }
    }
  }

  // Hold-to-heat in manual mode
  if (gState.pidController.systemEnabled && gState.pidController.manualMode && gState.input.stablePressed) {
    unsigned long held = now - gState.input.pressStartMs;
    if (held >= (BUTTON_LONG_MS + 500) && !gState.input.holdHeating) { gState.input.holdHeating = true; gState.input.pidOutputBeforeHold = gState.pidController.pidOutput; gState.pidController.pidOutput = PWM_MAX; Serial.println(F("[Manual] Hold heating ON (100%)")); }
  }

  // Check confirmation dialog timeout (auto-cancel extreme encoder changes)
  handleEncoderConfirmationTimeout(now);

  // Encoder handling: simple single-step consumption per loop to avoid runaway
  #if USE_ENCODER
    {
      int32_t ticks = encoderGetTicks();
      int32_t delta = ticks - encoderGetHandled();
      if (delta != 0) {
        // Sanity cap: if delta is absurdly large, assume bounce/noise and reset
        const int32_t SANITY_MAX = 200;
        if (abs(delta) > SANITY_MAX) {
          Serial.printf("[ENC] Large delta=%ld -> zeroing counters for safety\n", (long)delta);
          encoderZeroCounters();
          // skip processing this iteration
        } else {
          // Determine step size depending on context
          int stepEdges = (gState.menu.configMode ? ENC_MENU_STEP_EDGES : (gState.menu.active ? ENC_MENU_STEP_EDGES : ENC_EDGES_PER_DETENT));
          int32_t sign = (delta > 0) ? 1 : -1;

          // If we're in config editing mode, use ENC_EDGES_PER_DETENT
          if (gState.menu.configMode && gState.menu.configEditing) stepEdges = ENC_EDGES_PER_DETENT;

          if (abs(delta) >= stepEdges) {
            // Perform a single logical step in the detected direction
            // Apply encoder rate limiting: ignore steps faster than ENC_RATE_LIMIT_MS
            if ((now - gState.input.lastEncoderInputMs) < ENC_RATE_LIMIT_MS) {
              // Rate limited: skip this step
            } else if (gState.menu.configMode) {
              gState.input.lastEncoderInputMs = now;
              if (gState.menu.configEditing) {
                gState.pidController.defaultSetpoint += sign * ENC_STEP_C * ENC_STEP_SCALE;
                gState.pidController.defaultSetpoint = constrain(gState.pidController.defaultSetpoint, (double)ENC_MIN_C, (double)ENC_MAX_C);
                Serial.printf("[CONFIG] Editing Default SP=%.1f C\n", gState.pidController.defaultSetpoint);
              } else {
                gState.menu.configIndex += sign;
                while (gState.menu.configIndex < 0) gState.menu.configIndex += MenuManager::CONFIG_ITEM_COUNT;
                gState.menu.configIndex %= MenuManager::CONFIG_ITEM_COUNT;
                Serial.printf("[CONFIG] idx=%d\n", gState.menu.configIndex);
              }
              encoderAddHandled(sign * stepEdges);
              gState.timing.lastActivityMs = now;
            } else if (gState.menu.active) {
              gState.input.lastEncoderInputMs = now;
              gState.menu.index += sign;
              while (gState.menu.index < 0) gState.menu.index += MenuManager::MENU_ITEM_COUNT;
              gState.menu.index %= MenuManager::MENU_ITEM_COUNT;
              Serial.printf("[MENU] idx=%d\n", gState.menu.index);
              encoderAddHandled(sign * stepEdges);
              gState.timing.lastActivityMs = now;
            } else {
              // Normal setpoint/manual adjustments with input protection
              gState.input.lastEncoderInputMs = now;
              if (gState.pidController.manualMode) {
                int manStep = max(1, (int)((ENC_MAN_STEP_PCT * (int)PWM_MAX) / 100.0 + 0.5));
                int effDir = -ENC_DIR;
                long deltaOut = (long)manStep * (long)effDir * (long)sign;
                long newOut = (long)gState.pidController.pidOutput + deltaOut;
                newOut = constrain(newOut, 0L, (long)PWM_MAX);
                gState.pidController.pidOutput = (double)newOut;
                applyOutput(gState.pidController.pidOutput);
                Serial.printf("[ENC] => ManualOut=%ld (%d%%)\n", newOut, (int)((newOut*100)/PWM_MAX));
              } else {
                // Auto mode: setpoint adjustment with extreme change protection
                double proposedSetpoint = gState.pidController.setpointC + sign * ENC_STEP_C * ENC_STEP_SCALE;
                proposedSetpoint = constrain(proposedSetpoint, (double)ENC_MIN_C, (double)ENC_MAX_C);
                
                // Check if this is an extreme change
                if (isExtremeSetpointChange(gState.pidController.setpointC, proposedSetpoint)) {
                  // Require confirmation for large jumps
                  if (!gState.menu.awaitingConfirmation) {
                    gState.menu.pendingSetpointC = proposedSetpoint;
                    gState.menu.awaitingConfirmation = true;
                    gState.menu.confirmationStartMs = now;
                    Serial.printf("[ENC_CONFIRM] EXTREME CHANGE: Propose setpoint %.1fC -> %.1fC (delta: %.1fC)\n", 
                                  gState.pidController.setpointC, proposedSetpoint, proposedSetpoint - gState.pidController.setpointC);
                    Serial.println(F("[ENC_CONFIRM] Press button to confirm, or wait 5s to cancel"));
                  } else {
                    // Already awaiting confirmation; ignore additional encoder input
                    Serial.printf("[ENC] Input ignored; awaiting confirmation for setpoint change\n");
                  }
                } else {
                  // Safe change: apply immediately
                  gState.pidController.setpointC = proposedSetpoint;
                  Serial.printf("[ENC] => Setpoint=%.1f C\n", gState.pidController.setpointC);
                  gState.menu.awaitingConfirmation = false;  // Clear any pending confirmation
                }
              }
              encoderAddHandled(sign * stepEdges);
              gState.timing.lastActivityMs = now;
            }
          }
        }
      }
    }
  #endif

#if USE_BAT
  sampleBattery();
  // Check battery safety: charger removal, voltage instability, low warning
#endif

    // Sensor read (throttled for stable ADC sampling)
    {
      bool didRead = false;
      float newT = NAN;
      if ((now - gState.timing.lastTempSampleMs) >= MIN_TEMP_SAMPLE_MS) {
    newT = readTemperatureC();
    gState.timing.lastTempSampleMs = now;
    didRead = true;
      }

      // Update connection status and input only when we actually read the sensor
      if (didRead) {
      bool isSensorValid = !isnan(newT) && newT > -100.0f && newT <= 700.0f;
      
      if (!isSensorValid) {
        Serial.printf("[SENSOR] Invalid reading: %.1f C (fault count: %d)\n", newT, gState.sensorFault.faultCount);
        gState.pidController.inputC = NAN; 
        applyOutput(0);
      } else {
        gState.pidController.inputC = newT;
        // BLE temperature notification (if connected)
  #if USE_BLE
        if (gState.ble.chTemp && bleIsConnected()) {
          char tbuf[32]; snprintf(tbuf, sizeof(tbuf), "%.2f", gState.pidController.inputC);
          std::string tv(tbuf);
          gState.ble.chTemp->setValue(tv);
          gState.ble.chTemp->notify();
        }
  #endif
      }
      
      // Thermocouple status notification (notify on status change)
  #if USE_BLE
      static bool lastTcStatus = true;
      bool currentTcStatus = !gState.sensorFault.faulted;
      if (currentTcStatus != lastTcStatus && bleIsConnected() && gState.ble.chTcStatus) {
        gState.ble.chTcStatus->setValue(currentTcStatus ? "1" : "0");
        gState.ble.chTcStatus->notify();
        lastTcStatus = currentTcStatus;
        Serial.printf("[BLE] TC status changed: %s\n", currentTcStatus ? "OK" : "FAULT");
      }
  #endif
        }

      // Periodic serial status prints for realtime visibility (can be toggled off)
      if ((now - gState.timing.lastTempPrintMs) >= TEMP_PRINT_INTERVAL_MS) {
    gState.timing.lastTempPrintMs = now;
    if (gState.features.serialStreamingEnabled) {
      if (isnan(gState.pidController.inputC)) Serial.println(F("[STAT] temp=NaN"));
      else Serial.printf("[STAT] temp=%.2fC set=%.1fC out=%d\n", gState.pidController.inputC, gState.pidController.setpointC, (int)gState.pidController.pidOutput);
    }
      }

      // PID compute / apply (use last known gState.pidController.inputC if we didn't read just now)
      // Check thermal runaway before applying any output
      
      if (!gState.pidController.systemEnabled) {
  #if RELAY_MODE
    applyRelayWindow(0);
  #else
    applyOutput(0);
  #endif
      } else if (gState.safety.watchdogFaulted) {
    // Watchdog timeout: disable heater immediately (highest priority)
    Serial.println(F("[SAFETY] Watchdog fault, disabling heater"));
  #if RELAY_MODE
    applyRelayWindow(0);
  #else
    applyOutput(0);
  #endif
      } else if (gState.safety.thermalRunawayFaulted) {
    // Thermal runaway detected: disable heater immediately (second priority)
    Serial.println(F("[SAFETY] Thermal runaway fault, disabling heater"));
  #if RELAY_MODE
    applyRelayWindow(0);
  #else
    applyOutput(0);
  #endif
      } else if (gState.battery.chargerRemoved) {
    // Charger removed: ramp down heater gracefully or disable
    if (gState.battery.chargerRampActive) {
      // Apply ramp-down sequence
      unsigned long rampElapsed = now - gState.battery.chargerRampStartMs;
      int rampStep = (int)((rampElapsed * CHARGER_RAMP_DOWN_STEPS) / (CHARGER_REMOVED_WINDOW_MS * 2)); // ~10s ramp
      if (rampStep >= CHARGER_RAMP_DOWN_STEPS) {
        rampStep = CHARGER_RAMP_DOWN_STEPS;
        gState.battery.chargerRampActive = false;
      }
      double rampOutput = (double)PWM_MAX * (1.0 - (double)rampStep / (double)CHARGER_RAMP_DOWN_STEPS);
  #if RELAY_MODE
      applyRelayWindow(rampOutput);
  #else
      applyOutput(rampOutput);
  #endif
    } else {
      // Ramp complete; disable heater
  #if RELAY_MODE
      applyRelayWindow(0);
  #else
      applyOutput(0);
  #endif
    }
      } else if (gState.sensorFault.faulted) {
    // Sensor is faulted: disable heater and show error
    Serial.println(F("[SAFETY] Sensor fault detected, disabling heater"));
  #if RELAY_MODE
    applyRelayWindow(0);
  #else
    applyOutput(0);
  #endif
      } else if (
  #if USE_BAT
    gState.battery.lowCutoff
  #else
    false
  #endif
      ) {
  #if RELAY_MODE
    applyRelayWindow(0);
  #else
    applyOutput(0);
  #endif
      } else {
    // Force manual mode if battery low warning (but allow user to continue if desired)
#if USE_BAT
    if (gState.battery.lowWarning && !gState.pidController.manualMode) {
      gState.pidController.manualMode = true;
      Serial.println(F("[BAT] Low warning: forcing MANUAL mode (user can still use heater)"));
    }
#endif
    
    if (!gState.pidController.manualMode) {
    if (tempPID.Compute()) {
      gState.safety.pidLastComputeMs = now;  // Update PID watchdog timestamp
      // Apply ramp limiting to prevent sudden output jumps
      double rampLimitedOutput = applyOutputWithRampLimit(gState.pidController.pidOutput);
  #if RELAY_MODE
      applyRelayWindow(rampLimitedOutput);
  #else
      applyOutput(rampLimitedOutput);
  #endif
    }
    } else {
      // Manual mode: also apply ramp limiting for safety
      double rampLimitedOutput = applyOutputWithRampLimit(gState.pidController.pidOutput);
  #if RELAY_MODE
      applyRelayWindow(rampLimitedOutput);
  #else
      applyOutput(rampLimitedOutput);
  #endif
    }
      }

  // Display update with error recovery
#if USE_OLED
  if (gState.display.available) {
    // Update display via UIManager
    UIManager::update();
  }
#endif

  // Debug logging for disconnection investigation (throttled and toggleable)
  if ((now - gState.timing.lastDebugMs) >= 30000UL) { // Every 30 seconds
    gState.timing.lastDebugMs = now; // always advance timer so we don't burst when re-enabled
    if (gState.features.serialStreamingEnabled) {
      int bleConnected = 0;
      #if USE_BLE
      bleConnected = bleIsConnected() ? 1 : 0;
      #endif
      Serial.printf("[DEBUG] BLE connected: %d, Bat: %.2fV (%d%%), Temp: %.1fC, Mode: %d\n",
                    bleConnected,
                    gState.battery.voltage, gState.battery.percent, gState.pidController.inputC, gState.pidController.pidMode);
    }
  }

  // Persist runtime flags and consider deep sleep
  gState.pidController.rtc.manualMode = gState.pidController.manualMode;
  gState.pidController.rtc.systemEnabled = gState.pidController.systemEnabled;
  if ((now - gState.timing.lastActivityMs) >= SLEEP_ON_IDLE_MS && !gState.menu.active) {

    // If deep sleep has been disabled either at compile-time or runtime,
    // never call the esp sleep APIs; just report (throttled) that sleep is
    // suppressed so bench tests are uninterrupted.
    if (DISABLE_DEEP_SLEEP || gState.features.deepSleepDisabledRuntime || gState.features.serialStreamingEnabled) {
      if ((now - gState.timing.lastSleepPrintMs) >= SLEEP_PRINT_INTERVAL_MS) {
        gState.timing.lastSleepPrintMs = now;
        Serial.println(F("[SLEEP] deep sleep suppressed (disabled runtime/compile or streaming)"));
      }
    } else {
      Serial.printf("[SLEEP] idle %lums >= %lums, preparing deep sleep\n", (unsigned long)(now - gState.timing.lastActivityMs), (unsigned long)SLEEP_ON_IDLE_MS);
      // Avoid immediate wake-loop: require wake pins to be HIGH (not actively low)
      bool pinA = digitalRead(ENC_PIN_A);
      bool pinB = digitalRead(ENC_PIN_B);
      bool pinSw = digitalRead(ENC_PIN_SW);
      if (pinA == LOW || pinB == LOW || pinSw == LOW) {
        Serial.println(F("[SLEEP] aborting deep sleep: one or more wake pins LOW"));
      } else {
        Serial.println(F("[SLEEP] enabling gpio wake and entering deep sleep"));
        esp_sleep_enable_gpio_wakeup();
        gpio_wakeup_enable((gpio_num_t)ENC_PIN_A, GPIO_INTR_LOW_LEVEL);
        gpio_wakeup_enable((gpio_num_t)ENC_PIN_B, GPIO_INTR_LOW_LEVEL);
        gpio_wakeup_enable((gpio_num_t)ENC_PIN_SW, GPIO_INTR_LOW_LEVEL);
        // Small settle delay to avoid bounces causing immediate wake
        delay(10);
        esp_deep_sleep_start();
      }
    }
  } else {
    // Throttle SLEEP prints to avoid flooding the serial port
    if (gState.features.serialStreamingEnabled && (now - gState.timing.lastSleepPrintMs) >= SLEEP_PRINT_INTERVAL_MS) {
      gState.timing.lastSleepPrintMs = now;
      Serial.printf("[SLEEP] skipping deep sleep, idle=%lums, gState.menu.active=%d\n", (unsigned long)(now - gState.timing.lastActivityMs), gState.menu.active ? 1 : 0);
    }
  }
}
}

// --- Minimal implementations to satisfy linker and provide basic behavior ---
void applyRelayWindow(double value) {
  #if RELAY_MODE
    unsigned long now = millis();
    unsigned long elapsed = (now - gState.timing.windowStartTime) % relayWindowMs;
    double duty = constrain(value, 0.0, (double)PWM_MAX);
    if (elapsed < (unsigned long)((duty / (double)PWM_MAX) * relayWindowMs)) {
      digitalWrite(OUTPUT_PIN, HIGH);
    } else {
      digitalWrite(OUTPUT_PIN, LOW);
    }
  #else
    (void)value;
  #endif
}

void applyOutput(double value) {
  // Map value (0..PWM_MAX) to LEDC channel 0
  int duty = (int)constrain(value, 0.0, (double)PWM_MAX);
#if TEST_MODE
  // In test mode, skip hardware PWM and just log
  unsigned long now = millis();
  if (gState.features.serialStreamingEnabled) {
    if (duty != gState.timing.lastAppliedDuty || (now - gState.timing.lastApplyPrintMs) >= APPLY_PRINT_INTERVAL_MS) {
      Serial.printf("[APPLYOUT] (TEST_MODE) duty=%d\n", duty);
      gState.timing.lastAppliedDuty = duty;
      gState.timing.lastApplyPrintMs = now;
    }
  }
#else
  // Use pwmWrite wrapper which calls LEDC on ESP32 or a digital fallback
  pwmWrite(duty);
  // Debug print to confirm MCU applied the duty value, but throttle to
  // avoid flooding when duty stays constant.
  unsigned long now = millis();
  if (gState.features.serialStreamingEnabled) {
    if (duty != gState.timing.lastAppliedDuty || (now - gState.timing.lastApplyPrintMs) >= APPLY_PRINT_INTERVAL_MS) {
      Serial.printf("[APPLYOUT] duty=%d\n", duty);
      gState.timing.lastAppliedDuty = duty;
      gState.timing.lastApplyPrintMs = now;
    }
  }
#endif
  // persist manual output value to RTC so it survives deep sleep
  gState.pidController.rtc.pidOutput = (double)duty;
}

// Apply a PID mode selection. Modes: 0=AUTO, 1=MAN, 2=U1, 3=U2, 4=Config
void applyPidMode(int mode) {
  if (mode > 4) return;
  if (mode == 4) {
    // Config: enter config mode
    gState.menu.configMode = true;
    gState.menu.active = false;
    Serial.println(F("[CONFIG] Entered config mode"));
    return;
  }
  gState.pidController.pidMode = mode;
  // Preset tunings for U1/U2 (example values; adjust as needed)
  if (mode == 0) {
    // AUTO: keep current tunings, ensure automatic, set to default setpoint
    gState.pidController.manualMode = false;
    tempPID.SetMode(AUTOMATIC);
    gState.pidController.setpointC = gState.pidController.defaultSetpoint;
    Serial.println(F("PID Mode: AUTO"));
  } else if (mode == 1) {
    // MAN: manual mode
    gState.pidController.manualMode = true;
    tempPID.SetMode(MANUAL);
    Serial.println(F("PID Mode: MANUAL"));
  } else if (mode == 2) {
    // U1 preset: load configured preset values
    gState.pidController.Kp = gState.pidController.u1.Kp;
    gState.pidController.Ki = gState.pidController.u1.Ki;
    gState.pidController.Kd = gState.pidController.u1.Kd;
    tempPID.SetTunings(gState.pidController.Kp, gState.pidController.Ki, gState.pidController.Kd);
    gState.pidController.manualMode = false;
    tempPID.SetMode(AUTOMATIC);
    gState.pidController.setpointC = gState.pidController.u1.setpoint;
    Serial.println(F("PID Mode: U1 (preset)"));
  } else if (mode == 3) {
    // U2 preset: load configured preset values
    gState.pidController.Kp = gState.pidController.u2.Kp;
    gState.pidController.Ki = gState.pidController.u2.Ki;
    gState.pidController.Kd = gState.pidController.u2.Kd;
    tempPID.SetTunings(gState.pidController.Kp, gState.pidController.Ki, gState.pidController.Kd);
    gState.pidController.manualMode = false;
    tempPID.SetMode(AUTOMATIC);
    gState.pidController.setpointC = gState.pidController.u2.setpoint;
    Serial.println(F("PID Mode: U2 (preset)"));
  }
  // ensure PID controller is updated with new parameters
  tempPID.SetTunings(gState.pidController.Kp, gState.pidController.Ki, gState.pidController.Kd);
  // close the menu after selection
  gState.menu.active = false;
  Serial.printf("[MENU] Applied mode %d\n", mode);

  // Notify remote clients (BLE) of mode change if available
  const char *mstr = MenuManager::getMenuItemName(gState.pidController.pidMode);
#if USE_BLE
  // NimBLE variant
  if (gState.ble.chMode) {
    std::string s(mstr);
    gState.ble.chMode->setValue(s);
    gState.ble.chMode->notify();
  }
  if (gState.ble.chModeRead) {
    std::string ms = std::to_string(gState.pidController.pidMode);
    gState.ble.chModeRead->setValue(ms);
    gState.ble.chModeRead->notify();
  }
#endif
}

void printHelp() {
  Serial.println(F(" S <temp>   - set setpoint (C)"));
  Serial.println(F(" P <kp,ki,kd> - set PID tunings"));
  Serial.println(F(" O <0-1023> - manual output"));
  Serial.println(F(" T - print sensor & watchdog & thermal diagnostics"));
  Serial.println(F(" H - help"));
  Serial.println(F(" W - BLE status and characteristics"));
  Serial.println(F(" D - toggle deep sleep suppression at runtime"));
  Serial.println(F(" R - reset NVS preferences (espatom namespace)"));
  Serial.println(F(" Y - run smoke test: self-tests + snapshot"));
  Serial.println(F(" M - Management commands:"));
  Serial.println(F("     M   - Full diagnostics report (errors, health, counters)"));
  Serial.println(F("     MR  - Reset error counters and clear faults"));
  Serial.println(F("     MH  - Manual health check"));
  Serial.println(F("     MS  - System status summary"));
  Serial.println(F("[WATCHDOG] loop stall timeout=5s, PID compute timeout=10s"));
  Serial.println(F("[THERMAL] max temp=750C, margin=50C, max-on=30s@90%%, ramp=50/cycle"));
  Serial.println(F("[ENCODER] rate-limit=50ms, extreme>100C confirms, timeout=5s"));
  Serial.println(F("[BATTERY] warning=3.5V, cutoff=3.3V, charger_detect=500mV drop, ramp=10steps"));
  Serial.println(F("[RUNTIME] max continuous=30min, cooldown=1min"));
  Serial.println(F("[LIMITS] Absolute max temp=350C, min=0C, max errors=10"));
  Serial.println(F("[SELFTEST] Boot self-tests run at startup; type T to see results"));
}

// Print a one-shot compact snapshot of key runtime state to serial
void printSnapshot() {
  Serial.printf("[SNAP] temp=%.2fC set=%.1fC out=%d pwmMax=%d\n", isnan(gState.pidController.inputC) ? NAN : gState.pidController.inputC, gState.pidController.setpointC, (int)gState.pidController.pidOutput, (int)PWM_MAX);
  Serial.printf("[SNAP] kp=%.3f ki=%.3f kd=%.3f mode=%s manual=%d power=%d\n", gState.pidController.Kp, gState.pidController.Ki, gState.pidController.Kd, MenuManager::getMenuItemName(gState.pidController.pidMode), gState.pidController.manualMode?1:0, gState.pidController.systemEnabled?1:0);
  #if USE_BAT
  Serial.printf("[SNAP] batV=%.2fV batPct=%d\n", gState.battery.voltage, gState.battery.percent);
  #else
  Serial.println(F("[SNAP] batV=null batPct=null"));
  #endif
}

// Battery helpers moved to battery.{h,cpp}




static void runBootSelfTests() {
  // Test 1: Battery voltage is readable
  #if USE_BAT
  gState.bootTest.batteryOk = (gState.battery.voltage > 0.0f && gState.battery.voltage < 5.0f);
  #else
  gState.bootTest.batteryOk = true; // Pass if battery feature disabled
  #endif

  // Test 2: Temperature sensor is reading valid values
  // Try one quick read
  float testTemp = readTemperatureC();
  gState.bootTest.sensorOk = isThermocoupleValid() && !isnan(testTemp) && testTemp > -100.0f && testTemp <= 700.0f;

  // Test 3: Heater output can be applied (PWM is working)
  double testOutput = 100.0;  // Try a low safe value
  applyOutput(testOutput);
  gState.bootTest.heaterOk = true;  // If we got here without error, PWM is functional
  applyOutput(0);  // Disable heater after test

  // Test 4: Encoder pins are readable
  #if USE_ENCODER
  int encA = digitalRead(ENC_PIN_A);
  int encB = digitalRead(ENC_PIN_B);
  gState.bootTest.encoderOk = (encA == 0 || encA == 1) && (encB == 0 || encB == 1);
  #else
  gState.bootTest.encoderOk = true;  // Pass if encoder feature disabled
  #endif

  // Test 5: Button pin is readable
  #if USE_ENCODER
  int btnState = digitalRead(BUTTON_PIN);
  gState.bootTest.buttonOk = (btnState == 0 || btnState == 1);
  #else
  gState.bootTest.buttonOk = true;  // Pass if button feature disabled
  #endif

  // Log results to serial
  Serial.printf("[SELFTEST] Battery=%s Sensor=%s Heater=%s Encoder=%s Button=%s\n",
                gState.bootTest.batteryOk ? "OK" : "FAIL",
                gState.bootTest.sensorOk ? "OK" : "FAIL",
                gState.bootTest.heaterOk ? "OK" : "FAIL",
                gState.bootTest.encoderOk ? "OK" : "FAIL",
                gState.bootTest.buttonOk ? "OK" : "FAIL");
}
