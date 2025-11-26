
#include <Arduino.h>
#include <PID_v1.h>
#include <esp_sleep.h>  // For sleep functions
#include <esp_system.h>
#include <math.h>  // For thermistor Steinhart-Hart calculation
#include <Preferences.h>

#include "config.h"

#if USE_OLED
#include "oled.h"
#endif

// Firmware version string shown on boot OLED
static const char* FIRMWARE_VERSION = "ESPAtomizer v0.1";


#ifndef USE_BLE //Bluetooth Functionality
#define USE_BLE 1
#endif


#ifndef USE_WIFI //Wifi Functionality
#define USE_WIFI 0
#endif

#if USE_BLE
#include <NimBLEDevice.h>
#endif

#if USE_WIFI
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#endif

// Default to using MAX6675 thermocouple by default unless overridden
#ifndef USE_MAX6675
#define USE_MAX6675 1
#endif


#if USE_MAX6675
#include <SPI.h>
#endif


/*
ESPAtomizer Power, Battery, and Sleep Logic

- Startup Behavior:
  - Device always powers on when battery or USB is present (physical power switch ON).
  - Firmware boots and restores last saved state (setpoint, PID mode, output, etc.) from RTC memory.
  - If last state was OFF, device remains OFF; otherwise, resumes operation automatically.

- Battery Use and Unplugged Operation:
  - Device always uses battery power if available. If unplugged from USB/external supply, continues running on battery.
  - Remains operational on battery unless explicitly turned off via button, BLE, or web interface.
  - All features (PID, BLE, OLED, encoder, etc.) are available on battery power.

- Low Battery Handling:
  - If battery voltage drops below cutoff (BAT_CUTOFF_V), heating is disabled to protect the battery, but device remains responsive for status, BLE, and UI.

- Sleep Modes:
  - Device enters deep sleep after inactivity (default: 60s, configurable via SLEEP_ON_IDLE_MS).
  - Deep sleep is aborted if any wake pins (encoder A/B/SW) are LOW, preventing unwanted sleep during user interaction.
  - On wake, device restores previous state from RTC memory and resumes operation.
  - Device will not enter deep sleep if menu is active or recent activity is detected.

- Power-Off Logic:
  - Device only shuts down if user turns it off (button press, BLE command, or web control).
  - Otherwise, runs on battery until depleted or manually powered off.

- Physical Power Switch:
  - If hardware power switch is installed inline with battery, switching OFF fully disconnects power and turns off device.
  - Switching ON restores power and device boots immediately, regardless of previous software state.
*/



#if USE_BLE
// BLE UUIDs (randomly generated)
// forward-declare applyPidMode used by BLE callbacks
void applyPidMode(int mode);
static NimBLEServer* bleServer = nullptr;
static NimBLECharacteristic *chEnable, *chSetpoint, *chKp, *chKi, *chKd, *chMode, *chTemp, *chOut, *chBat, *chModeRead, *chDefaultSp, *chUnit;
// BLE characteristics to control preheat/preset behaviour (U1/U2)
static NimBLECharacteristic *chPreU1Enabled, *chPreU1Ms, *chPreU2Enabled, *chPreU2Ms;
static const char* BLE_SVC_UUID = "b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a";
static const char* UUID_ENABLE  = "3f1a0001-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_SETPOINT= "3f1a0002-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_KP      = "3f1a0003-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_KI      = "3f1a0004-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_KD      = "3f1a0005-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_MODE    = "3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_TEMP    = "3f1a0007-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_OUT     = "3f1a0008-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_BAT     = "3f1a0009-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_MODE_READ = "3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8002";
static const char* UUID_DEFAULT_SP = "3f1a000a-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_UNIT = "3f1a000b-2a8d-4a54-8f2f-b7cd2b4b8001"; // "C" or "F"
static const char* UUID_PREU1_ENABLED = "3f1a00b1-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_PREU1_MS      = "3f1a00b2-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_PREU2_ENABLED = "3f1a00b3-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_PREU2_MS      = "3f1a00b4-2a8d-4a54-8f2f-b7cd2b4b8001";

// Defer the full BLE callback and setup definitions until after the
// static globals are declared. Provide lightweight forward declarations
// here so other code (e.g. prototypes) can refer to them.
class ChCallbacks;
static void setupBLE();
// Last BLE connection event (human-readable): updated on connect/disconnect
static char lastBleEvent[32] = "none";
// Last encoder initialization summary (filled by encoderInit)
static char lastEncoderInfo[64] = "enc:none";

// MAX6675 thermocouple library selection and fallback stub
#ifndef MAX6675_INCLUDE
#define MAX6675_INCLUDE 1
#endif

#if USE_MAX6675
  // Manual override: 1 = Adafruit <max6675.h>, 2 = Rob Tillaart <MAX6675.h>, 0 = auto-detect
  #if MAX6675_INCLUDE == 1
    #include <max6675.h>
    #define HAVE_MAX6675_LIB 1
    #define MAX6675_INCLUDE_NAME "<max6675.h>"
  #elif MAX6675_INCLUDE == 2
    #include <MAX6675.h>
    #define HAVE_MAX6675_LIB 1
    #define MAX6675_INCLUDE_NAME "<MAX6675.h>"
  #else
    #if defined(__has_include)
      #if __has_include(<max6675.h>)
        #include <max6675.h>
        #define HAVE_MAX6675_LIB 1
        #define MAX6675_INCLUDE_NAME "<max6675.h>"
      #elif __has_include(<MAX6675.h>)
        #include <MAX6675.h>
        #define HAVE_MAX6675_LIB 1
        #define MAX6675_INCLUDE_NAME "<MAX6675.h>"
      #endif
    #endif
  #endif

  #ifndef HAVE_MAX6675_LIB
    #warning "MAX6675 library not found – building with stub (temperature will read NaN). Install library to enable readings."
    class MAX6675 {
     public:
      MAX6675(int, int, int) {}
      double readCelsius() { return NAN; }
    };
  #endif

  static MAX6675 thermocouple(MAX6675_SCK_PIN, MAX6675_CS_PIN, MAX6675_SO_PIN);
#endif // USE_MAX6675

// Explicit ESP32-C6 GPIO assignments (avoid relying on Dn aliases across cores)
// XIAO ESP32-C6 (aligned to Seeed D-pin labels)
//  - Thermistor node: GPIO0 (A0 / D0)
//  - Rotary encoder:  A=GPIO0 (D0), B=GPIO1 (D1), SW=GPIO2 (D2, active-low)

// PID parameters (tune as needed)
static double Kp = 10.0, Ki = 0.5, Kd = 50.0; // conservative starting point

// PID variables
static double setpointC = 200.0; // target temperature in C (updated default)
static double inputC = 0.0;     // measured temperature
static double pidOutput = 0.0;  // controller output

// PID controller (DIRECT means increase output when error positive)
static PID tempPID(&inputC, &pidOutput, &setpointC, Kp, Ki, Kd, DIRECT);

// RTC memory for deep sleep persistence
RTC_DATA_ATTR static double rtcSetpointC = 200.0;
RTC_DATA_ATTR static double rtcKp = 10.0;
RTC_DATA_ATTR static double rtcKi = 0.5;
RTC_DATA_ATTR static double rtcKd = 50.0;
RTC_DATA_ATTR static bool rtcManualMode = false;
RTC_DATA_ATTR static bool rtcSystemEnabled = false;
RTC_DATA_ATTR static int rtcPidMode = 0;
// Persist manual output across deep sleep
RTC_DATA_ATTR static double rtcPidOutput = 0.0;
// Persisted presets for U1/U2
RTC_DATA_ATTR static double rtcU1Kp = 12.0;
RTC_DATA_ATTR static double rtcU1Ki = 0.7;
RTC_DATA_ATTR static double rtcU1Kd = 60.0;
RTC_DATA_ATTR static double rtcU1Sp = 220.0;
RTC_DATA_ATTR static double rtcU2Kp = 8.0;
RTC_DATA_ATTR static double rtcU2Ki = 0.3;
RTC_DATA_ATTR static double rtcU2Kd = 40.0;
RTC_DATA_ATTR static double rtcU2Sp = 180.0;
RTC_DATA_ATTR static double rtcDefaultSetpoint = 200.0;
// Persisted Bluetooth settings
RTC_DATA_ATTR static bool rtcBleEnabled = true;
RTC_DATA_ATTR static int rtcBleNameIndex = 0;
RTC_DATA_ATTR static unsigned long rtcBleAdvIntervalMs = 200UL; // informational only
// Persisted OLED indicator enable/disable
RTC_DATA_ATTR static bool rtcBleOledIndicator = true;
RTC_DATA_ATTR static bool rtcTempUnitIsC = true; // persisted: true=C, false=F
// Optional preheat settings for U1/U2 presets (persisted in RTC)
RTC_DATA_ATTR static bool rtcU1PreheatEnabled = false;
RTC_DATA_ATTR static unsigned long rtcU1PreheatMs = 3000UL; // default 3s
RTC_DATA_ATTR static bool rtcU2PreheatEnabled = false;
RTC_DATA_ATTR static unsigned long rtcU2PreheatMs = 2000UL; // default 2s

// Persist last boot error message (survives deep-sleep / soft resets)
RTC_DATA_ATTR static char rtcLastBootError[64] = "";

// Mode and control
static bool manualMode = false;         // false=PID auto, true=manual
static unsigned long loopIntervalMs = 1000; // PID compute/sample time
static unsigned long lastLoopMs = 0;
static bool systemEnabled = (INIT_POWER_ON != 0); // power switch state (default OFF)
static bool remoteControlEnabled = true;          // allow remote control; can gate if needed

// Simple on-device menu state (opened by long-press of encoder switch)
static bool menuActive = false;
static int menuIndex = 0;
// PID mode: 0=AUTO, 1=MAN, 2=U1, 3=U2, 4=Config
static int pidMode = 0;
// Top-level menu items. 'BT' is a Bluetooth settings entry that opens
// a small on-device submenu for basic BLE configuration.
static const char* MENU_ITEMS[] = { "AUTO", "MAN", "U1", "U2", "Config" };
static const int MENU_ITEM_COUNT = (sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]));
static bool configMode = false;

// (BT submenu removed — BLE controls moved into Config menu)

// Consolidated on-device Config items (includes BLE controls previously in BT submenu)
static const char* CONFIG_ITEMS[] = { "Default SP", "Unit", "BLE ON", "Name", "AdvInt", "OLED", "Save", "Factory Reset", "Forget", "Back" };
static const int CONFIG_ITEM_COUNT = (sizeof(CONFIG_ITEMS)/sizeof(CONFIG_ITEMS[0]));

// Config navigation state
static int configIndex = 0;
static bool configEditing = false; // true when editing a value (e.g., Default SP)
// BLE indicator animation state (used to blink on connect/disconnect)
static unsigned long bleAnimExpireMs = 0;
static bool bleAnimIsConnect = false;
// Preset names to choose from on-device
static const char* BLE_NAME_PRESETS[] = { "Adamizer", "ESPAtom", "Atomizer" };
static const int BLE_NAME_PRESET_COUNT = (sizeof(BLE_NAME_PRESETS)/sizeof(BLE_NAME_PRESETS[0]));

// Activity tracking for sleep
static unsigned long lastActivityMs = 0;
static unsigned long lastDebugMs = 0;

// Minimum allowed sample interval (ms) to respect MAX6675 conversion time (~200-300ms)
#ifndef MIN_TEMP_SAMPLE_MS
#define MIN_TEMP_SAMPLE_MS 300UL
#endif
#if USE_MAX6675
// Track if the K-type thermocouple (via MAX6675) appears connected
static bool tcConnected = false;
// Hysteresis counters to avoid flapping when readings bounce between NaN
// and valid values due to wiring/noise. Require two consecutive good
// reads to mark connected, and two consecutive bad reads to mark
// disconnected.
static uint8_t tcGoodCount = 0;
static uint8_t tcBadCount = 0;

// Timestamp for last thermocouple sample and serial print
static unsigned long lastTempSampleMs = 0;
static unsigned long lastTempPrintMs = 0;
#ifndef TEMP_PRINT_INTERVAL_MS
// Increase temperature/status print interval (ms) to reduce serial traffic
#define TEMP_PRINT_INTERVAL_MS 5000UL
#endif
#endif

// When false, periodic serial prints are suppressed. Use serial command 'Z' to toggle.
static bool serialStreamingEnabled = true;
// Throttle for SLEEP messages (ms)
#ifndef SLEEP_PRINT_INTERVAL_MS
#define SLEEP_PRINT_INTERVAL_MS 30000UL
#endif
static unsigned long lastSleepPrintMs = 0;

// Throttle for APPLYOUT prints: only print when duty changes or this interval passes
#ifndef APPLY_PRINT_INTERVAL_MS
#define APPLY_PRINT_INTERVAL_MS 2000UL
#endif
static int lastAppliedDuty = -1;
static unsigned long lastApplyPrintMs = 0;

// Button state for debouncing
static bool btnStable = BUTTON_ACTIVE_LOW ? true : false; // stores last stable raw level
static bool btnLastRead = false;                           // last instantaneous raw read
static unsigned long btnLastChangeMs = 0;                  // for debounce timing
static bool btnStablePressed = false;                      // last stable pressed state
static unsigned long btnPressStartMs = 0;                  // press start time for long-press detection
static bool btnLongHandled = false;                        // whether long-press action was already handled while held

// Manual mode hold-to-heat state
static bool btnHoldHeating = false;
static double pidOutputBeforeHold = 0;

// Preheat runtime state (activated when selecting U1/U2 with preheat enabled)
static bool preheatActive = false;
static unsigned long preheatEndMs = 0;
static double pidOutputBeforePreheat = 0;
static int preheatWhich = 0; // 0=none,1=U1,2=U2


#if USE_ENCODER
#include "encoder.h"
#endif

#if USE_BAT
#include "battery.h"
#endif

// PWM/analog range derived from resolution
static const uint16_t PWM_MAX = (1U << PWM_RES_BITS) - 1U; // e.g., 1023 for 10-bit
// PWM channel to use for LEDC (ESP32)
static const int PWM_CHANNEL = 0;

// Forward declarations
void applyOutput(double value);
void applyRelayWindow(double value);
void handleSerial();
void printStatus();
void printHelp();
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
  // expose different helper signatures — e.g. the Arduino "esp32-hal-ledc.h"
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
#if 0
// The `applyPidMode` declaration is provided earlier (C++ linkage).
// Leaving this disabled to avoid duplicate/conflicting linkage declarations.
#endif
#if USE_OLED
void updateDisplay();
#endif

// WiFi/webserver/OTA removed for BLE-only builds.
// Provide minimal no-op helpers so callers in setup()/loop() can remain unchanged.
static inline void wifiLoopHandle() { (void)0; }
static inline void wifiDoSetup() { (void)0; }

// (ArduinoBLE removed) NimBLE is used above for BLE functionality

#if USE_BLE
// Full BLE callback and advertising setup; placed after static globals so
// the callback can reference file-scope `static` variables directly.
class ChCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) {
    std::string uuid = c->getUUID().toString();
    std::string val = c->getValue();
    auto toF = [&](const std::string &s){ return atof(s.c_str()); };
    if (uuid == UUID_ENABLE) {
      if (!val.empty()) {
        systemEnabled = (val[0] != '0');
        if (!systemEnabled) { tempPID.SetMode(MANUAL); pidOutput = 0; applyOutput(0); }
        else { tempPID.SetMode(manualMode ? MANUAL : AUTOMATIC); }
      }
    } else if (uuid == UUID_SETPOINT) {
      setpointC = toF(val);
      // If writing setpoint while in a preset mode, persist the preset setpoint
      if (pidMode == 2) {
        rtcU1Sp = setpointC;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f", rtcU1Sp);
        prefs.putString("u1_sp", buf); prefs.end();
        Serial.println(F("[PREF] U1 SP saved from BLE"));
      } else if (pidMode == 3) {
        rtcU2Sp = setpointC;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f", rtcU2Sp);
        prefs.putString("u2_sp", buf); prefs.end();
        Serial.println(F("[PREF] U2 SP saved from BLE"));
      }
    }
    else if (uuid == UUID_KP) {
      Kp = toF(val); tempPID.SetTunings(Kp, Ki, Kd);
      // If currently using a preset (U1/U2), persist the tuning to that preset
      if (pidMode == 2) {
        rtcU1Kp = Kp;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", rtcU1Kp);
        prefs.putString("u1_kp", buf); prefs.end();
        Serial.println(F("[PREF] U1 Kp saved from BLE"));
      } else if (pidMode == 3) {
        rtcU2Kp = Kp;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", rtcU2Kp);
        prefs.putString("u2_kp", buf); prefs.end();
        Serial.println(F("[PREF] U2 Kp saved from BLE"));
      }
    }
    else if (uuid == UUID_KI) {
      Ki = toF(val); tempPID.SetTunings(Kp, Ki, Kd);
      if (pidMode == 2) {
        rtcU1Ki = Ki;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", rtcU1Ki);
        prefs.putString("u1_ki", buf); prefs.end();
        Serial.println(F("[PREF] U1 Ki saved from BLE"));
      } else if (pidMode == 3) {
        rtcU2Ki = Ki;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", rtcU2Ki);
        prefs.putString("u2_ki", buf); prefs.end();
        Serial.println(F("[PREF] U2 Ki saved from BLE"));
      }
    }
    else if (uuid == UUID_KD) {
      Kd = toF(val); tempPID.SetTunings(Kp, Ki, Kd);
      if (pidMode == 2) {
        rtcU1Kd = Kd;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", rtcU1Kd);
        prefs.putString("u1_kd", buf); prefs.end();
        Serial.println(F("[PREF] U1 Kd saved from BLE"));
      } else if (pidMode == 3) {
        rtcU2Kd = Kd;
        Preferences prefs; prefs.begin("espatom", false);
        char buf[32]; snprintf(buf, sizeof(buf), "%.3f", rtcU2Kd);
        prefs.putString("u2_kd", buf); prefs.end();
        Serial.println(F("[PREF] U2 Kd saved from BLE"));
      }
    }
    else if (uuid == UUID_MODE) {
      if (!val.empty()) {
        std::string s = val;
        for (auto &ch : s) ch = toupper(ch);
        if (s.rfind("SAVE:", 0) == 0) {
          std::string p = s.substr(5);
          if (p == "U1") {
            unsigned long bootStart = millis();
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0,0);
            display.println(F("ESPAtomizer boot"));
            // Firmware version
            display.print(F("FW: "));
            display.println(FIRMWARE_VERSION);
            // Battery
            if (!isnan(batteryVoltage)) {
              char bbuf[12]; dtostrf(batteryVoltage, 0, 2, bbuf);
              display.print(F("BAT: ")); display.print(bbuf); display.print(F("V "));
              display.print(batteryPercent); display.println(F("%"));
            } else {
              display.println(F("BAT: --V --%"));
            }
            // PID tunings
            display.print(F("PID: "));
            char kbuf[40]; snprintf(kbuf, sizeof(kbuf), "Kp=%.1f Ki=%.2f Kd=%.1f", Kp, Ki, Kd);
            display.println(kbuf);
            // MAX6675 status (one-shot read)
        #if USE_MAX6675
            {
              double t = thermocouple.readCelsius();
              if (!isnan(t)) {
                char tbuf[16]; dtostrf(t, 0, 1, tbuf);
                display.print(F("MAX6675: OK ")); display.println(tbuf);
              } else {
                display.println(F("MAX6675: FAIL"));
              }
            }
        #else
            display.println(F("MAX6675: disabled"));
        #endif
            // Encoder pins + interrupt mapping
            int ia = digitalPinToInterrupt(ENC_PIN_A);
            int ib = digitalPinToInterrupt(ENC_PIN_B);
            display.print(F("ENC A:")); display.print(ENC_PIN_A); display.print(F(" i:")); display.println(ia);
            display.print(F("ENC B:")); display.print(ENC_PIN_B); display.print(F(" i:")); display.println(ib);
            // BLE status
        #if USE_BLE
            display.print(F("BLE: ")); display.println(rtcBleEnabled?"ON":"OFF");
          #if USE_BLE
                // Last BLE event (connect/disconnect timestamp) and uptime
                display.print(F("BLE last: ")); display.println(lastBleEvent);
          #endif
                unsigned long upsecs = millis() / 1000UL;
                char upbuf[24]; snprintf(upbuf, sizeof(upbuf), "UP: %lus", upsecs);
                display.println(upbuf);
        #else
            display.println(F("BLE: disabled"));
        #endif
            display.display();
            // Keep the boot screen visible for a total of ~3500ms
            unsigned long elapsed = millis() - bootStart;
            if (elapsed < 3500UL) delay(3500UL - elapsed);
            display.clearDisplay(); display.display();
            // Persist current U1 presets when SAVE:U1 is received
            {
              Preferences prefs; prefs.begin("espatom", false);
              char buf[32];
              snprintf(buf, sizeof(buf), "%.3f", rtcU1Kp); prefs.putString("u1_kp", buf);
              snprintf(buf, sizeof(buf), "%.3f", rtcU1Ki); prefs.putString("u1_ki", buf);
              snprintf(buf, sizeof(buf), "%.3f", rtcU1Kd); prefs.putString("u1_kd", buf);
              snprintf(buf, sizeof(buf), "%.1f", rtcU1Sp); prefs.putString("u1_sp", buf);
              prefs.putString("u1_pre_en", rtcU1PreheatEnabled ? "1" : "0");
              snprintf(buf, sizeof(buf), "%lu", rtcU1PreheatMs); prefs.putString("u1_pre_ms", buf);
              prefs.end();
              Serial.println(F("[PREF] U1 presets saved from BLE SAVE:U1"));
            }
      }
          else if (p == "U2") {
            // Persist current U2 presets when SAVE:U2 is received
            Preferences prefs; prefs.begin("espatom", false);
            char buf[32];
            snprintf(buf, sizeof(buf), "%.3f", rtcU2Kp); prefs.putString("u2_kp", buf);
            snprintf(buf, sizeof(buf), "%.3f", rtcU2Ki); prefs.putString("u2_ki", buf);
            snprintf(buf, sizeof(buf), "%.3f", rtcU2Kd); prefs.putString("u2_kd", buf);
            snprintf(buf, sizeof(buf), "%.1f", rtcU2Sp); prefs.putString("u2_sp", buf);
            prefs.putString("u2_pre_en", rtcU2PreheatEnabled ? "1" : "0");
            snprintf(buf, sizeof(buf), "%lu", rtcU2PreheatMs); prefs.putString("u2_pre_ms", buf);
            prefs.end();
            Serial.println(F("[PREF] U2 presets saved from BLE SAVE:U2"));
          }
    } else if (uuid == UUID_DEFAULT_SP) {
      if (!val.empty()) {
        double v = toF(val);
        rtcDefaultSetpoint = v;
        Serial.printf("[BLE] Default SP set to %.1fC\n", rtcDefaultSetpoint);
        // Persist default SP
        {
          Preferences prefs;
          prefs.begin("espatom", false);
          char buf[16]; snprintf(buf, sizeof(buf), "%.1f", rtcDefaultSetpoint);
          prefs.putString("def_sp", buf);
          prefs.end();
          Serial.println(F("[PREF] Default SP saved from BLE"));
        }
      }
    } else if (uuid == UUID_PREU1_MS) {
      if (!val.empty()) {
        unsigned long v = (unsigned long)strtoul(val.c_str(), nullptr, 10);
        if (v > 0) rtcU1PreheatMs = v;
        Serial.printf("[BLE] rtcU1PreheatMs=%lums\n", rtcU1PreheatMs);
      }
    } else if (uuid == UUID_PREU2_ENABLED) {
      if (!val.empty()) {
        rtcU2PreheatEnabled = (val[0] != '0');
        Serial.printf("[BLE] rtcU2PreheatEnabled=%d\n", rtcU2PreheatEnabled?1:0);
        // Persist U2 preheat enabled
        {
          Preferences prefs; prefs.begin("espatom", false);
          prefs.putString("u2_pre_en", rtcU2PreheatEnabled?"1":"0"); prefs.end();
          Serial.println(F("[PREF] U2 preheat enabled saved"));
        }
      }
    } else if (uuid == UUID_PREU2_MS) {
      if (!val.empty()) {
        unsigned long v = (unsigned long)strtoul(val.c_str(), nullptr, 10);
        if (v > 0) rtcU2PreheatMs = v;
        Serial.printf("[BLE] rtcU2PreheatMs=%lums\n", rtcU2PreheatMs);
        // Persist U2 preheat ms
        {
          Preferences prefs; prefs.begin("espatom", false);
          char buf[16]; snprintf(buf, sizeof(buf), "%lu", rtcU2PreheatMs);
          prefs.putString("u2_pre_ms", buf); prefs.end();
          Serial.println(F("[PREF] U2 preheat ms saved"));
        }
      }
    } else if (uuid == UUID_UNIT) {
      if (!val.empty()) {
        char c = val[0];
        if (c == 'C' || c == 'c' || c == '0') rtcTempUnitIsC = true;
        else if (c == 'F' || c == 'f' || c == '1') rtcTempUnitIsC = false;
        Serial.printf("[BLE] Temp unit set to %s\n", rtcTempUnitIsC?"C":"F");
        // Notify connected clients of the new unit
        #if USE_BLE
        if (chUnit && bleServer && bleServer->getConnectedCount() > 0) {
          std::string s = rtcTempUnitIsC ? "C" : "F";
          chUnit->setValue(s);
          chUnit->notify();
        }
        #endif
        // Persist unit choice received over BLE
        {
          Preferences prefs;
          prefs.begin("espatom", false);
          prefs.putString("unit", rtcTempUnitIsC ? "1" : "0");
          prefs.end();
          Serial.println(F("[PREF] Unit saved from BLE write"));
        }
      }
    } else if (uuid == UUID_OUT) { pidOutput = constrain(atoi(val.c_str()), 0, (int)PWM_MAX); }
  }
};

static void setupBLE(){
  NimBLEDevice::init("Adamizer");
  Serial.println(F("BLE device initialized with name 'Adamizer'"));
  bleServer = NimBLEDevice::createServer();
  NimBLEService* svc = bleServer->createService(BLE_SVC_UUID);

  auto propsRW = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE;
  chEnable   = svc->createCharacteristic(UUID_ENABLE, propsRW);
  chSetpoint = svc->createCharacteristic(UUID_SETPOINT, propsRW);
  chKp       = svc->createCharacteristic(UUID_KP, propsRW);
  chKi       = svc->createCharacteristic(UUID_KI, propsRW);
  chKd       = svc->createCharacteristic(UUID_KD, propsRW);
  chMode     = svc->createCharacteristic(UUID_MODE, propsRW);
  chOut      = svc->createCharacteristic(UUID_OUT, propsRW);
  chTemp     = svc->createCharacteristic(UUID_TEMP, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  chBat      = svc->createCharacteristic(UUID_BAT, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  chModeRead = svc->createCharacteristic(UUID_MODE_READ, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  chDefaultSp= svc->createCharacteristic(UUID_DEFAULT_SP, propsRW);
  // Unit characteristic: 'C' or 'F' - allows read/write and notify to sync remote app
  chUnit = svc->createCharacteristic(UUID_UNIT, propsRW | NIMBLE_PROPERTY::NOTIFY);
  // Preheat control characteristics (U1 / U2)
  chPreU1Enabled = svc->createCharacteristic(UUID_PREU1_ENABLED, propsRW);
  chPreU1Ms      = svc->createCharacteristic(UUID_PREU1_MS, propsRW);
  chPreU2Enabled = svc->createCharacteristic(UUID_PREU2_ENABLED, propsRW);
  chPreU2Ms      = svc->createCharacteristic(UUID_PREU2_MS, propsRW);

  static ChCallbacks cb; // persists
  chEnable->setCallbacks(&cb);
  chSetpoint->setCallbacks(&cb);
  chKp->setCallbacks(&cb);
  chKi->setCallbacks(&cb);
  chKd->setCallbacks(&cb);
  chMode->setCallbacks(&cb);
  chOut->setCallbacks(&cb);
  chPreU1Enabled->setCallbacks(&cb);
  chPreU1Ms->setCallbacks(&cb);
  chPreU2Enabled->setCallbacks(&cb);
  chPreU2Ms->setCallbacks(&cb);
  chUnit->setCallbacks(&cb);

  svc->start();
  Serial.println(F("BLE service started"));
  // Configure security/bonding so iOS can pair and remember the peripheral.
  // Enable bonding, MITM (display/yes-no), and LE Secure Connections where supported.
  NimBLEDevice::setSecurityAuth(true, true, true);
  // IO capability: display + yes/no confirmation to support numeric comparison
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO);

  // Attempt to register security callbacks if the NimBLE security callback
  // API is available in this build. Try multiple common header locations so
  // builds that place NimBLE headers under subfolders are detected as well.
#define HAVE_NIMBLE_SECURITY_HDR 1

// Define minimal types and class for security callbacks
#include <cstdint>
struct ble_gap_conn_desc {
    uint16_t conn_handle;
    // Minimal definition
};

class NimBLESecurityCallbacks {
public:
    virtual ~NimBLESecurityCallbacks() {}
    virtual uint32_t onPassKeyRequest();
    virtual void onPassKeyNotify(uint32_t pass_key);
    virtual bool onConfirmPIN(uint32_t pass_key);
    virtual bool onSecurityRequest();
    virtual void onAuthenticationComplete(ble_gap_conn_desc *desc);
};

#if defined(HAVE_NIMBLE_SECURITY_HDR)
  class RuntimeSecurityCallbacks : public NimBLESecurityCallbacks {
    public:
      uint32_t onPassKeyRequest() override {
        Serial.println(F("[BLE_SEC] PassKeyRequest (none)"));
        return 0;
      }
      void onPassKeyNotify(uint32_t pass_key) override {
        char buf[16]; snprintf(buf, sizeof(buf), "%06u", pass_key);
        Serial.printf("[BLE_SEC] PassKeyNotify %s\n", buf);
      }
      bool onConfirmPIN(uint32_t pass_key) override {
        Serial.printf("[BLE_SEC] ConfirmPIN %06u -> accept\n", (unsigned)pass_key);
        return true; // auto-confirm numeric comparison
      }
      bool onSecurityRequest() override {
        Serial.println(F("[BLE_SEC] SecurityRequest -> accept (bond)"));
        return true; // accept bonding requests
      }
      void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
        if (!desc) {
          Serial.println(F("[BLE_SEC] Authentication complete (no desc)"));
          return;
        }
        Serial.printf("[BLE_SEC] Auth complete: conn_handle=%u\n", desc->conn_handle);
        Serial.println(F("[BLE_SEC] Authentication finished (check bonding status)"));
      }
  };

  // Register the callbacks with NimBLEDevice if the registration API exists
  // (most NimBLE-Arduino builds provide NimBLEDevice::setSecurityCallbacks).
  #if defined(NimBLEDevice)
    NimBLEDevice::setSecurityCallbacks(new RuntimeSecurityCallbacks());
  #endif
#else
  Serial.println(F("[BLE_SEC] NimBLE security header not found; skipping security callbacks"));
#endif

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  // Build advertisement data with the local name so scanners (iOS) show it.
  NimBLEAdvertisementData advData;
  advData.setName("Adamizer");
  // Set a GAP Appearance value to help central OSes classify the device.
  // 0x0340 is commonly used for Thermometer-like devices; adjust if you prefer.
  #if defined(__has_include)
    // Only call if the method exists in this NimBLE build — this is conservative
    // and will be skipped if the library variant doesn't provide setAppearance.
    #if __has_include(<NimBLEAdvertisement.h>) || __has_include(<nimble/NimBLEAdvertisement.h>)
      // Some NimBLE-Arduino variants expose setAppearance on NimBLEAdvertisementData
      advData.setAppearance(0x0340);
    #endif
  #else
    // Fallback: attempt to set appearance (may be a no-op on some builds)
    advData.setAppearance(0x0340);
  #endif
  // Attach advertisement data (name). Service UUIDs are added separately.
  adv->setAdvertisementData(advData);
  adv->addServiceUUID(BLE_SVC_UUID);
  // Ensure advertising is connectable so iOS will allow pairing from Settings
  // and prefer no min connection interval to make discovery responsive.
  // Use `setConnectableMode` which is present in some NimBLE-Arduino builds.
  // If your NimBLE variant uses a different API, we'll skip the call to avoid
  // compile errors.
  #if defined(NimBLEAdvertising) || defined(NimBLEDevice)
    // prefer calling setConnectableMode if available in this build
    adv->setConnectableMode(1);
  #endif
  // setMinPreferred(0) removes preferred connection params that some central
  // stacks may otherwise try to enforce; it's a common NimBLE helper.
  #if defined(NimBLEAdvertising) || defined(NimBLEDevice)
    // guard to avoid compile issues on trimmed-down toolchains
    adv->setMinPreferred(0);
  #endif
  adv->start();
  Serial.println(F("BLE advertising started (NimBLE advanced settings applied)"));
  Serial.print(F("BLE service UUID: "));
  Serial.println(BLE_SVC_UUID);
  Serial.print(F("BLE advertising status: "));
  Serial.println(adv->isAdvertising() ? "ACTIVE" : "INACTIVE");
  class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
      Serial.println(F("BLE client connected"));
      // Trigger a brief connect animation on OLED
      bleAnimExpireMs = millis() + 1000UL; // 1s animation
      bleAnimIsConnect = true;
      // Record a simple last-connection event for diagnostics/boot display
      snprintf(lastBleEvent, sizeof(lastBleEvent), "conn@%lus", millis() / 1000UL);
    }
    void onDisconnect(NimBLEServer* pServer) {
      Serial.println(F("BLE client disconnected"));
      // Trigger a brief disconnect animation on OLED
      bleAnimExpireMs = millis() + 1000UL; // 1s animation
      bleAnimIsConnect = false;
      // Record the disconnect event timestamp for diagnostics
      snprintf(lastBleEvent, sizeof(lastBleEvent), "disc@%lus", millis() / 1000UL);
      NimBLEDevice::getAdvertising()->start();
      Serial.println(F("BLE advertising restarted after disconnect"));
    }
  };
  bleServer->setCallbacks(new ServerCallbacks());

  // --- Characteristic security note ---
  // To ensure iOS performs system pairing/bonding (so the device appears
  // in Settings with an (i) details button), key control characteristics
  // should require encryption/authentication. Different NimBLE-Arduino
  // versions expose different APIs to mark characteristics as encrypted.
  // Adding those calls here is intentionally omitted to keep compatibility;
  // if your toolchain exposes `setAccessPermissions` or `setSecurity` on
  // `NimBLECharacteristic`, we can enable read/write encryption on the
  // control characteristics (e.g. chEnable, chSetpoint, chKp, chKi, chKd)
  // to force the OS to present the bonding prompt. If you'd like, I can
  // add those calls guarded by `__has_include` checks for your NimBLE
  // headers — tell me if you want me to attempt that next.

    // Attempt to mark key control characteristics as requiring encryption/authentication
    // so that accessing them from a central will trigger system pairing on iOS.
    // Try multiple paths for the common NimBLE GATT header and enable this
    // step only when the GATT permission macros are available.
  #if defined(__has_include)
    #if __has_include(<host/ble_gatts.h>)
      #define HAVE_NIMBLE_GATTS_HDR 1
      #include <host/ble_gatts.h>
    #elif __has_include(<nimble/ble_gatts.h>)
      #define HAVE_NIMBLE_GATTS_HDR 1
      #include <nimble/ble_gatts.h>
    #elif __has_include(<nimble/host/ble_gatts.h>)
      #define HAVE_NIMBLE_GATTS_HDR 1
      #include <nimble/host/ble_gatts.h>
    #elif __has_include(<host/host/ble_gatts.h>)
      #define HAVE_NIMBLE_GATTS_HDR 1
      #include <host/host/ble_gatts.h>
    #endif
  #endif

  // Developer-local fallback: include the esp32-arduino-libs copy of ble_gatt.h
  // when it's buried under the Arduino15 packages tree (common on Windows).
  // This uses a full path specific to this machine; it's guarded so other
  // environments are unaffected.
  #if !defined(HAVE_NIMBLE_GATTS_HDR)
    #if defined(__has_include)
      #if __has_include("C:/Users/adinj/AppData/Local/Arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-f1a1df9b-v3/esp32/esp32c6/include/bt/host/nimble/nimble/nimble/host/include/host/ble_gatt.h")
        #define HAVE_NIMBLE_GATTS_HDR 1
        #include "C:/Users/adinj/AppData/Local/Arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-f1a1df9b-v3/esp32/esp32c6/include/bt/host/nimble/host/include/host/ble_gatts.h"
      #endif
    #endif
  #endif

  #ifndef BLE_GATT_PERM_READ
  #define BLE_GATT_PERM_READ 0x01
  #endif
  #ifndef BLE_GATT_PERM_WRITE
  #define BLE_GATT_PERM_WRITE 0x02
  #endif
  #ifndef BLE_GATT_PERM_READ_ENCRYPTED
  #define BLE_GATT_PERM_READ_ENCRYPTED 0x04
  #endif
  #ifndef BLE_GATT_PERM_WRITE_ENCRYPTED
  #define BLE_GATT_PERM_WRITE_ENCRYPTED 0x08
  #endif

  #if defined(HAVE_NIMBLE_GATTS_HDR)
    // Declare the function if not already declared in headers
    extern "C" int ble_gatts_set_attr_perm(uint16_t attr_handle, uint8_t perm);
    // Set per-characteristic access permissions to require encryption for bonding
    // This forces iOS to prompt for pairing when accessing control characteristics
    ble_gatts_set_attr_perm(chEnable->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chSetpoint->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chKp->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chKi->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chKd->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chMode->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chOut->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chTemp->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED); // read-only
    ble_gatts_set_attr_perm(chBat->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED); // read-only
    ble_gatts_set_attr_perm(chModeRead->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED); // read-only
    ble_gatts_set_attr_perm(chDefaultSp->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chPreU1Enabled->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chPreU1Ms->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chPreU2Enabled->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chPreU2Ms->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    ble_gatts_set_attr_perm(chUnit->getHandle() + 1, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED | BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED);
    Serial.println(F("[BLE_SEC] Set characteristic access permissions to require encryption for bonding"));
  #else
    Serial.println(F("[BLE_SEC] NimBLE GATTS header not found; cannot set characteristic access permissions"));

  #endif
}
#endif // USE_BLE

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000); // Give serial time to initialize
  while (!Serial && millis() < 3000) {
    delay(10); // Wait up to 3 seconds for serial
  }
  // Print persisted boot error if present
  if (rtcLastBootError[0] != '\0') {
    Serial.printf("[RTC] Last boot error: %s\n", rtcLastBootError);
  }

  // Load persisted non-volatile config (Preferences/NVS) so settings survive power cycles
  {
    Preferences prefs;
    prefs.begin("espatom", true);
    String v;
    v = prefs.getString("def_sp", ""); if (v.length()) rtcDefaultSetpoint = atof(v.c_str());
    v = prefs.getString("unit", ""); if (v.length()) rtcTempUnitIsC = (v.charAt(0) == '1');
    v = prefs.getString("ble_en", ""); if (v.length()) rtcBleEnabled = (v.charAt(0) == '1');
    v = prefs.getString("ble_name", ""); if (v.length()) rtcBleNameIndex = atoi(v.c_str());
    v = prefs.getString("ble_adv", ""); if (v.length()) rtcBleAdvIntervalMs = strtoul(v.c_str(), nullptr, 10);
    v = prefs.getString("oled_ind", ""); if (v.length()) rtcBleOledIndicator = (v.charAt(0) == '1');
    prefs.end();
    Serial.println(F("[PREF] Loaded persisted config (if present)."));
  }
  // Initialize battery early so we can display voltage on the boot OLED
#if USE_BAT
  initBattery();
  sampleBattery();
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
    if (!isnan(batteryVoltage)) {
      char bbuf[12]; dtostrf(batteryVoltage, 0, 2, bbuf);
      display.print(F("BAT: ")); display.print(bbuf); display.print(F("V "));
      display.print(batteryPercent); display.println(F("%"));
    } else {
      display.println(F("BAT: --V --%"));
    }

    // Encoder pins + interrupt mapping and short init summary
    int ia = digitalPinToInterrupt(ENC_PIN_A);
    int ib = digitalPinToInterrupt(ENC_PIN_B);
    display.print(F("ENC A:")); display.print(ENC_PIN_A); display.print(F(" i:")); display.println(ia);
    display.print(F("ENC B:")); display.print(ENC_PIN_B); display.print(F(" i:")); display.println(ib);
    display.print(F("ENC: ")); display.println(lastEncoderInfo);

    // Quick thermocouple one-shot check (if enabled)
    bool tc_ok = true;
  #if USE_MAX6675
    double tcv = thermocouple.readCelsius();
    if (isnan(tcv)) tc_ok = false;
  #else
    tc_ok = false; // thermistor support removed; mark as absent
  #endif

    // Aggregate errors/warnings
    String errors = "";
    String warns = "";
    // Battery: NaN = ADC missing -> warning. Very low voltage (<0.5V) = likely ADC open/short/error -> treat as error.
    if (isnan(batteryVoltage)) {
      warns += "BAT:ADC? ";
    } else {
      if (batteryVoltage < 0.5) errors += "BAT:0? ";
      else if (batteryVoltage < BAT_MIN_V) warns += "BAT:LOW ";
    }
    if (!tc_ok) warns += "TC:NO ";
    if (ia == -1 || ib == -1) warns += "ENC_IRQ? ";

    // BLE presence check (best-effort)
  #if USE_BLE
    if (!rtcBleEnabled) warns += "BLE:off ";
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
      strncpy(rtcLastBootError, errors.c_str(), sizeof(rtcLastBootError)-1);
      rtcLastBootError[sizeof(rtcLastBootError)-1] = '\0';
      delay(2000);
    } else {
      // Clear persisted error if boot succeeded
      rtcLastBootError[0] = '\0';
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
      const char* devName = BLE_NAME_PRESETS[rtcBleNameIndex];
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
      delay(1000);
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
  Serial.printf("Pins: OUT=%d(D6), ENC_A=%d(D0), ENC_B=%d(D1), ENC_SW=%d(D2), SDA=%d(D4), SCL=%d(D5), MAX SCK=%d(D10), CS=%d(D9), SO=%d(D8), BAT=%d(D3)\n",
                OUTPUT_PIN, ENC_PIN_A, ENC_PIN_B, ENC_PIN_SW, OLED_SDA, OLED_SCL,
                MAX6675_SCK_PIN, MAX6675_CS_PIN, MAX6675_SO_PIN, BAT_PIN);

  // Temperature sensor setup: using MAX6675 K-type (SPI); no ADC pin required
  // Sensor init message
#if USE_MAX6675
  Serial.printf("Using MAX6675 K-type thermocouple (SPI) SCK=%d CS=%d SO=%d\n",
                MAX6675_SCK_PIN, MAX6675_CS_PIN, MAX6675_SO_PIN);
#else
  Serial.println(F("Using NTC thermistor (NTC-MF52-103, 10k@25C, B=3950)"));
#endif

  // Initialize output/PWM (wrapper handles cross-core differences)
  pwmInit();

  // Initialize output off
  applyOutput(0);

  // PID setup
  tempPID.SetOutputLimits(0, PWM_MAX); // match PWM range
  tempPID.SetSampleTime(loopIntervalMs);
  tempPID.SetMode(AUTOMATIC);

  windowStartTime = millis();

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
  btnLastRead = digitalRead(BUTTON_PIN);
  btnStable = btnLastRead;
  btnLastChangeMs = millis();
  btnStablePressed = BUTTON_ACTIVE_LOW ? (btnStable == LOW) : (btnStable == HIGH);
  Serial.printf("Button initial raw=%d (active-%s)\n", (int)btnLastRead, BUTTON_ACTIVE_LOW ? "LOW" : "HIGH");

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
    Serial.println(F("[OLED] Init failed — trying swapped SDA/SCL pins..."));
    initOk = tryInitOLED(OLED_SCL, OLED_SDA);
    if (initOk) Serial.println(F("[OLED] Responded after swapping SDA/SCL (software swap)"));
  }
  if (!initOk) {
    Serial.println(F("[OLED] not found; continuing without display."));
    displayAvailable = false;
  }
#endif

  Serial.println(F("Setup complete!"));
  printHelp();

  // WiFi setup helper (noop if USE_WIFI==0)
  wifiDoSetup();

#if USE_BLE
  setupBLE();
#endif

  // Check wake-up cause and restore RTC state
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
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
    // Restore state from RTC memory
    setpointC = rtcSetpointC;
    Kp = rtcKp;
    Ki = rtcKi; 
    Kd = rtcKd;
    manualMode = rtcManualMode;
    systemEnabled = rtcSystemEnabled;
    tempPID.SetTunings(Kp, Ki, Kd);
    tempPID.SetMode(manualMode ? MANUAL : AUTOMATIC);
    // restore pidMode
    pidMode = rtcPidMode;
    applyPidMode(pidMode);
    // restore output (manual output) from RTC
    pidOutput = rtcPidOutput;
    applyOutput(pidOutput);
    Serial.println(F("State restored from RTC memory"));
  } else {
    // First boot, initialize RTC with defaults
    rtcSetpointC = setpointC;
    rtcKp = Kp;
    rtcKi = Ki;
    rtcKd = Kd;
    rtcManualMode = manualMode;
    rtcPidMode = pidMode;
    // initialize U1/U2 presets in RTC (defaults)
    rtcU1Kp = 12.0; rtcU1Ki = 0.7; rtcU1Kd = 60.0; rtcU1Sp = 220.0;
    rtcU2Kp = 8.0;  rtcU2Ki = 0.3; rtcU2Kd = 40.0; rtcU2Sp = 180.0;
    rtcSystemEnabled = systemEnabled;
    // initialize persisted pidOutput
    rtcPidOutput = pidOutput;
  }

  lastActivityMs = millis();  // Initialize activity timer
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
  // MAX6675 read; returns NaN on open thermocouple in most libraries
  double tc = thermocouple.readCelsius();
  if (isnan(tc)) return NAN;
  return (float)tc;
}

void loop() {
  // Main loop: handle serial, network, input, sensor, PID, display, and sleep
  handleSerial();
  wifiLoopHandle();

  const unsigned long now = millis();

  // LED status / charging indicator
  static unsigned long ledBlinkTimer = 0;
  static bool ledBlinkState = false;
  bool charging = false;
#if USE_BAT
  charging = (batteryPercent < 100 && batteryVoltage > BAT_MIN_V);
#endif
  if (systemEnabled) {
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
  if (buttonRaw != (int)btnLastRead) { btnLastChangeMs = now; btnLastRead = buttonRaw; }
  if ((now - btnLastChangeMs) >= BUTTON_DEBOUNCE_MS) {
    if ((bool)buttonRaw != btnStable) {
      btnStable = (bool)buttonRaw;
      bool stablePressed = BUTTON_ACTIVE_LOW ? (btnStable == LOW) : (btnStable == HIGH);
      // Press edge: record start and reset long-press handled flag
      if (stablePressed && !btnStablePressed) {
        btnPressStartMs = now; lastActivityMs = now; btnLongHandled = false; Serial.println(F("[BTN] Pressed"));
      }

      // While held, if threshold reached and not yet handled, trigger long-press action now
      if (stablePressed && !btnLongHandled) {
        unsigned long heldNow = now - btnPressStartMs;
        if (heldNow >= BUTTON_LONG_MS) {
          btnLongHandled = true;
          // long press toggles menu immediately when threshold crossed
          menuActive = !menuActive;
          // ensure config mode is closed when toggling menus
          configMode = false;
          if (menuActive) { menuIndex = pidMode; Serial.println(F("[MENU] Opened")); }
          else { Serial.println(F("[MENU] Closed")); }
          if (btnHoldHeating) { pidOutput = pidOutputBeforeHold; btnHoldHeating = false; }
        }
      }

      // Release edge: handle short-press actions only if long-press wasn't already handled
      if (!stablePressed && btnStablePressed) {
        unsigned long held = now - btnPressStartMs;
        Serial.printf("[BTN] Released after %lums\n", held);
        if (!btnLongHandled) {
          // short press: select menu item, BT submenu action, or toggle power
          if (configMode) {
            // In config mode: if currently editing a value, confirm/save on short press.
            if (configEditing) {
              configEditing = false;
              Serial.printf("[CONFIG] Saved Default SP=%.1f C\n", rtcDefaultSetpoint);
              // Persist default setpoint to Preferences so it survives power cycles
              {
                Preferences prefs;
                prefs.begin("espatom", false);
                char buf[16]; snprintf(buf, sizeof(buf), "%.1f", rtcDefaultSetpoint);
                prefs.putString("def_sp", buf);
                prefs.end();
                Serial.println(F("[PREF] Default setpoint saved"));
              }
            } else {
              // Execute action for selected config item
              switch (configIndex) {
                case 0: // Default SP: enter editing mode
                  configEditing = true;
                  Serial.println(F("[CONFIG] Editing Default SP (use encoder)"));
                  break;
                case 1: // Unit toggle
                  rtcTempUnitIsC = !rtcTempUnitIsC;
                  Serial.printf("[CONFIG] Temp unit toggled -> %s\n", rtcTempUnitIsC?"C":"F");
                  // Persist unit choice
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    prefs.putString("unit", rtcTempUnitIsC ? "1" : "0");
                    prefs.end();
                    Serial.println(F("[PREF] Unit saved"));
                  }
                  #if USE_BLE
                  if (chUnit && bleServer && bleServer->getConnectedCount() > 0) {
                    std::string s = rtcTempUnitIsC ? "C" : "F";
                    chUnit->setValue(s);
                    chUnit->notify();
                  }
                  #endif
                  break;
                case 2: // BLE ON toggle
                  rtcBleEnabled = !rtcBleEnabled;
                  Serial.printf("[BT] BLE enabled=%d\n", rtcBleEnabled?1:0);
                  // Persist BLE enabled flag
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    prefs.putString("ble_en", rtcBleEnabled ? "1" : "0");
                    prefs.end();
                    Serial.println(F("[PREF] BLE enabled saved"));
                  }
                  if (rtcBleEnabled) {
                    setupBLE();
                  } else {
                    if (bleServer) {
                      NimBLEDevice::getAdvertising()->stop();
                      Serial.println(F("[BT] Advertising stopped"));
                    }
                  }
                  break;
                case 3: // Name: cycle presets
                  rtcBleNameIndex = (rtcBleNameIndex + 1) % BLE_NAME_PRESET_COUNT;
                  Serial.printf("[BT] BLE name set to %s\n", BLE_NAME_PRESETS[rtcBleNameIndex]);
                  // Persist chosen name index
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    char buf[8]; snprintf(buf, sizeof(buf), "%d", rtcBleNameIndex);
                    prefs.putString("ble_name", buf);
                    prefs.end();
                    Serial.println(F("[PREF] BLE name index saved"));
                  }
                  if (NimBLEDevice::getAdvertising()) {
                    NimBLEAdvertisementData a; a.setName(BLE_NAME_PRESETS[rtcBleNameIndex]);
                    NimBLEDevice::getAdvertising()->setAdvertisementData(a);
                  }
                  break;
                case 4: // AdvInt cycle
                  if (rtcBleAdvIntervalMs == 200UL) rtcBleAdvIntervalMs = 100UL;
                  else if (rtcBleAdvIntervalMs == 100UL) rtcBleAdvIntervalMs = 1000UL;
                  else rtcBleAdvIntervalMs = 200UL;
                  Serial.printf("[BT] Advert interval preset = %lums (restart advertising to apply)\n", rtcBleAdvIntervalMs);
                  // Persist adv interval
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    char buf[16]; snprintf(buf, sizeof(buf), "%lu", rtcBleAdvIntervalMs);
                    prefs.putString("ble_adv", buf);
                    prefs.end();
                    Serial.println(F("[PREF] BLE adv interval saved"));
                  }
                  break;
                case 5: // OLED indicator toggle
                  rtcBleOledIndicator = !rtcBleOledIndicator;
                  Serial.printf("[BT] OLED indicator enabled=%d\n", rtcBleOledIndicator?1:0);
                  // Persist OLED indicator preference
                  {
                    Preferences prefs;
                    prefs.begin("espatom", false);
                    prefs.putString("oled_ind", rtcBleOledIndicator ? "1" : "0");
                    prefs.end();
                    Serial.println(F("[PREF] OLED indicator saved"));
                  }
                  #if USE_OLED
                  if (displayAvailable && !rtcBleOledIndicator) {
                    display.clearDisplay();
                    display.display();
                    Serial.println(F("[OLED] Indicator disabled; display cleared"));
                  } else if (displayAvailable && rtcBleOledIndicator) {
                    Serial.println(F("[OLED] Indicator enabled"));
                  }
                  #endif
                  break;
                case 6: // Forget bonds
                  Serial.println(F("[BT] Forget bonds requested"));
                  // Attempt several conditional APIs to clear persisted bonds/store.
#if defined(__has_include)
  // Try several common locations for the NimBLE store header
  #if __has_include(<nimble/ble_store.h>)
    #define HAVE_NIMBLE_STORE_HDR 1
    #include <nimble/ble_store.h>
  #elif __has_include(<nimble/store/ble_store.h>)
    #define HAVE_NIMBLE_STORE_HDR 1
    #include <nimble/store/ble_store.h>
  #elif __has_include(<host/ble_store.h>)
    #define HAVE_NIMBLE_STORE_HDR 1
    #include <host/ble_store.h>
  #elif __has_include(<ble_store/ble_store.h>)
    #define HAVE_NIMBLE_STORE_HDR 1
    #include <ble_store/ble_store.h>
  #elif __has_include(<host/store/ble_store.h>)
    #define HAVE_NIMBLE_STORE_HDR 1
    #include <host/store/ble_store.h>
  #endif
#endif

// Developer-local fallback for ble_store.h in the Arduino15 esp32 package
#if !defined(HAVE_NIMBLE_STORE_HDR)
  #if defined(__has_include)
    // Exact path discovered on this machine for esp32c6 IDF release. Use it
    // when available so we can access the NimBLE host store helpers.
    #if __has_include("C:/Users/adinj/AppData/Local/Arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-f1a1df9b-v3/esp32c6/include/bt/host/nimble/nimble/nimble/host/include/host/ble_store.h")
      // Avoid including the full header (it pulls other includes that may
      // not be on the Arduino sketch include path). Instead declare the
      // minimal host APIs we need as C prototypes so we can call them.
      #define HAVE_NIMBLE_STORE_HDR 1
      extern "C" {
        int ble_store_clear(void);
        int ble_store_util_delete_all(int type, const void *key);
      }
      #ifndef BLE_STORE_OBJ_TYPE_PEER_SEC
        #define BLE_STORE_OBJ_TYPE_PEER_SEC 2
      #endif
    #endif
  #endif
#endif

#if defined(HAVE_NIMBLE_STORE_HDR)
  ble_store_clear();
  Serial.println(F("[BT] ble_store_clear() called to remove stored bonds/CCCDs"));
#else
  Serial.println(F("[BT] NimBLE store header not found; cannot clear bonds programmatically in this build"));
#endif
                  break;
                case 7: // Back
                  configMode = false;
                  Serial.println(F("[CONFIG] Exited config mode"));
                  break;
                default:
                  break;
              }
            }
          } else if (menuActive) {
            if (menuIndex >= 0 && menuIndex < MENU_ITEM_COUNT) applyPidMode(menuIndex);
          } else {
            systemEnabled = !systemEnabled;
            if (!systemEnabled) {
              tempPID.SetMode(MANUAL); pidOutput = 0;
#if RELAY_MODE
              applyRelayWindow(0);
#else
              applyOutput(0);
#endif
              Serial.println(F("Power: OFF"));
              if (btnHoldHeating) btnHoldHeating = false;
            } else {
              tempPID.SetMode(manualMode ? MANUAL : AUTOMATIC);
              Serial.println(F("Power: ON"));
            }
          }
        }
            else if (cmd == 'F') {
              // Factory reset / clear persisted prefs via serial: "F", "FACTORY" or "CLEARPREFS"
              String arg = line;
              arg.toUpperCase();
              if (arg == "F" || arg == "FACTORY" || arg == "CLEARPREFS") {
                Serial.println(F("[SERIAL] Factory reset requested via serial - clearing Preferences"));
                {
                  Preferences prefs;
                  prefs.begin("espatom", false);
                  prefs.clear();
                  prefs.end();
                }
                Serial.println(F("[PREF] Preferences cleared via serial factory reset"));
                #if USE_OLED
                if (displayAvailable) {
                  display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
                  display.setCursor(0, (OLED_HEIGHT/2)-4); display.println(F("Factory reset")); display.display(); delay(900);
                  display.clearDisplay(); display.display();
                }
                #endif
                delay(200);
                esp_restart();
              } else {
                Serial.println(F("Usage: FACTORY | CLEARPREFS | F"));
              }
            }
        if (btnHoldHeating) { pidOutput = pidOutputBeforeHold; btnHoldHeating = false; Serial.println(F("[Manual] Hold heating OFF")); }
      }
      btnStablePressed = stablePressed;
    }
  }

  // If the button is stably pressed, detect long-press threshold while held
  if (btnStablePressed && !btnLongHandled) {
    unsigned long heldNow = now - btnPressStartMs;
    if (heldNow >= BUTTON_LONG_MS) {
      btnLongHandled = true;
      // long press toggles menu immediately when threshold crossed
      menuActive = !menuActive;
      // ensure config mode is closed when toggling menus
      configMode = false;
      if (menuActive) { menuIndex = pidMode; Serial.println(F("[MENU] Opened")); }
      else { Serial.println(F("[MENU] Closed")); }
      if (btnHoldHeating) { pidOutput = pidOutputBeforeHold; btnHoldHeating = false; }
    }
  }

  // Hold-to-heat in manual mode
  if (systemEnabled && manualMode && btnStablePressed) {
    unsigned long held = now - btnPressStartMs;
    if (held >= (BUTTON_LONG_MS + 500) && !btnHoldHeating) { btnHoldHeating = true; pidOutputBeforeHold = pidOutput; pidOutput = PWM_MAX; Serial.println(F("[Manual] Hold heating ON (100%)")); }
  }

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
          int stepEdges = (configMode ? ENC_MENU_STEP_EDGES : (menuActive ? ENC_MENU_STEP_EDGES : ENC_EDGES_PER_DETENT));
          int32_t sign = (delta > 0) ? 1 : -1;

          // If we're in config editing mode, use ENC_EDGES_PER_DETENT
          if (configMode && configEditing) stepEdges = ENC_EDGES_PER_DETENT;

          if (abs(delta) >= stepEdges) {
            // Perform a single logical step in the detected direction
            if (configMode) {
              if (configEditing) {
                rtcDefaultSetpoint += sign * ENC_STEP_C * ENC_STEP_SCALE;
                rtcDefaultSetpoint = constrain(rtcDefaultSetpoint, (double)ENC_MIN_C, (double)ENC_MAX_C);
                Serial.printf("[CONFIG] Editing Default SP=%.1f C\n", rtcDefaultSetpoint);
              } else {
                configIndex += sign;
                while (configIndex < 0) configIndex += CONFIG_ITEM_COUNT;
                configIndex %= CONFIG_ITEM_COUNT;
                Serial.printf("[CONFIG] idx=%d\n", configIndex);
              }
              encoderAddHandled(sign * stepEdges);
              lastActivityMs = now;
            } else if (menuActive) {
              menuIndex += sign;
              while (menuIndex < 0) menuIndex += MENU_ITEM_COUNT;
              menuIndex %= MENU_ITEM_COUNT;
              Serial.printf("[MENU] idx=%d\n", menuIndex);
              encoderAddHandled(sign * stepEdges);
              lastActivityMs = now;
            } else {
              // Normal setpoint/manual adjustments
              if (manualMode) {
                int manStep = max(1, (int)((ENC_MAN_STEP_PCT * (int)PWM_MAX) / 100.0 + 0.5));
                int effDir = -ENC_DIR;
                long deltaOut = (long)manStep * (long)effDir * (long)sign;
                long newOut = (long)pidOutput + deltaOut;
                newOut = constrain(newOut, 0L, (long)PWM_MAX);
                pidOutput = (double)newOut;
                applyOutput(pidOutput);
                Serial.printf("[ENC] => ManualOut=%ld (%d%%)\n", newOut, (int)((newOut*100)/PWM_MAX));
              } else {
                setpointC += sign * ENC_STEP_C * ENC_STEP_SCALE;
                setpointC = constrain(setpointC, (double)ENC_MIN_C, (double)ENC_MAX_C);
                Serial.printf("[ENC] => Setpoint=%.1f C\n", setpointC);
              }
              encoderAddHandled(sign * stepEdges);
              lastActivityMs = now;
            }
          }
        }
      }
  #endif

  // Battery sampling
#if USE_BAT
  sampleBattery();
#endif

    // Sensor read (throttled to respect MAX6675 conversion time)
    {
      bool didRead = false;
      float newT = NAN;
      if ((now - lastTempSampleMs) >= MIN_TEMP_SAMPLE_MS) {
    newT = readTemperatureC();
    lastTempSampleMs = now;
    didRead = true;
      }

      // Update connection status and input only when we actually read the sensor
  #if USE_MAX6675
      if (didRead) {
    // Debug: print raw reading for diagnostics
    if (serialStreamingEnabled) Serial.printf("[TDBG] raw_read=%.3f\n", isnan(newT) ? NAN : newT);
    bool good = !isnan(newT) && newT > -100.0f && newT <= 700.0f;
    if (good) {
      tcGoodCount = (tcGoodCount < 255) ? tcGoodCount + 1 : tcGoodCount;
      tcBadCount = 0;
    } else {
      tcBadCount = (tcBadCount < 255) ? tcBadCount + 1 : tcBadCount;
      tcGoodCount = 0;
    }
    bool prevTc = tcConnected;
    if (tcGoodCount >= 2) tcConnected = true;
    else if (tcBadCount >= 2) tcConnected = false;
    if (tcConnected != prevTc) Serial.printf("[TC] %s\n", tcConnected ? "connected" : "disconnected");
      }
  #endif

      if (didRead) {
      if (isnan(newT) || newT <= -100.0f || newT > 700.0f) {
        Serial.println(F("[ERR] Sensor fault or out-of-range reading."));
        inputC = NAN; applyOutput(0);
      } else {
        inputC = newT;
        // BLE temperature notification (if connected)
  #if USE_BLE
        if (chTemp && bleServer && bleServer->getConnectedCount() > 0) {
          char tbuf[32]; snprintf(tbuf, sizeof(tbuf), "%.2f", inputC);
          std::string tv(tbuf);
          chTemp->setValue(tv);
          chTemp->notify();
        }
  #endif
      }
        }

      // Periodic serial status prints for realtime visibility (can be toggled off)
      if ((now - lastTempPrintMs) >= TEMP_PRINT_INTERVAL_MS) {
    lastTempPrintMs = now;
    if (serialStreamingEnabled) {
      if (isnan(inputC)) Serial.println(F("[STAT] temp=NaN"));
      else Serial.printf("[STAT] temp=%.2fC set=%.1fC out=%d tcConn=%d\n", inputC, setpointC, (int)pidOutput, (int)(tcConnected?1:0));
    }
      }

      // PID compute / apply (use last known inputC if we didn't read just now)
      if (!systemEnabled) {
  #if RELAY_MODE
    applyRelayWindow(0);
  #else
    applyOutput(0);
  #endif
      } else if (
  #if USE_BAT
    batteryLow
  #else
    false
  #endif
      ) {
  #if RELAY_MODE
    applyRelayWindow(0);
  #else
    applyOutput(0);
  #endif
      } else if (preheatActive) {
    // During a preset preheat period, force full duty and skip PID
    if (millis() < preheatEndMs) {
      applyOutput((double)PWM_MAX);
    } else {
      // Preheat finished: hand control back to PID
      preheatActive = false;
      Serial.println(F("[PREHEAT] finished, resuming PID"));
      // restore previous manual output as a starting point for PID
      pidOutput = pidOutputBeforePreheat;
    }
      } else if (!manualMode) {
    if (tempPID.Compute()) {
  #if RELAY_MODE
      applyRelayWindow(pidOutput);
  #else
      applyOutput(pidOutput);
  #endif
    }
      } else {
  #if RELAY_MODE
    applyRelayWindow(pidOutput);
  #else
    applyOutput(pidOutput);
  #endif
      }
    }

  // Display update
#if USE_OLED
  if (displayAvailable) updateDisplay();
#endif

  // Debug logging for disconnection investigation (throttled and toggleable)
  if ((now - lastDebugMs) >= 30000UL) { // Every 30 seconds
    lastDebugMs = now; // always advance timer so we don't burst when re-enabled
    if (serialStreamingEnabled) {
      int bleConnected = 0;
      #if USE_BLE
      bleConnected = bleServer ? (bleServer->getConnectedCount() > 0 ? 1 : 0) : 0;
      #endif
      Serial.printf("[DEBUG] BLE connected: %d, Bat: %.2fV (%d%%), Temp: %.1fC, Mode: %d\n",
                    bleConnected,
                    batteryVoltage, batteryPercent, inputC, pidMode);
    }
  }

  // Persist runtime flags and consider deep sleep
  rtcManualMode = manualMode;
  rtcSystemEnabled = systemEnabled;
  if ((now - lastActivityMs) >= SLEEP_ON_IDLE_MS && !menuActive) {

    // If deep sleep has been disabled either at compile-time or runtime,
    // never call the esp sleep APIs; just report (throttled) that sleep is
    // suppressed so bench tests are uninterrupted.
    if (DISABLE_DEEP_SLEEP || disableDeepSleepRuntime || serialStreamingEnabled) {
      if ((now - lastSleepPrintMs) >= SLEEP_PRINT_INTERVAL_MS) {
        lastSleepPrintMs = now;
        Serial.println(F("[SLEEP] deep sleep suppressed (disabled runtime/compile or streaming)"));
      }
    } else {
      Serial.printf("[SLEEP] idle %lums >= %lums, preparing deep sleep\n", (unsigned long)(now - lastActivityMs), (unsigned long)SLEEP_ON_IDLE_MS);
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
    if (serialStreamingEnabled && (now - lastSleepPrintMs) >= SLEEP_PRINT_INTERVAL_MS) {
      lastSleepPrintMs = now;
      Serial.printf("[SLEEP] skipping deep sleep, idle=%lums, menuActive=%d\n", (unsigned long)(now - lastActivityMs), menuActive ? 1 : 0);
    }
  }
      }
    }

  // --- Minimal implementations to satisfy linker and provide basic behavior ---
  void applyRelayWindow(double value) {
  #if RELAY_MODE
    unsigned long now = millis();
    unsigned long elapsed = (now - windowStartTime) % relayWindowMs;
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
  // Use pwmWrite wrapper which calls LEDC on ESP32 or a digital fallback
  pwmWrite(duty);
    // Debug print to confirm MCU applied the duty value, but throttle to
    // avoid flooding when duty stays constant.
    unsigned long now = millis();
    if (serialStreamingEnabled) {
      if (duty != lastAppliedDuty || (now - lastApplyPrintMs) >= APPLY_PRINT_INTERVAL_MS) {
        Serial.printf("[APPLYOUT] duty=%d\n", duty);
        lastAppliedDuty = duty;
        lastApplyPrintMs = now;
      }
    }
    // persist manual output value to RTC so it survives deep sleep
    rtcPidOutput = (double)duty;
  }

  // Apply a PID mode selection. Modes: 0=AUTO, 1=MAN, 2=U1, 3=U2, 4=Config
  void applyPidMode(int mode) {
    if (mode < 0) return;
    if (mode > 4) return;
    if (mode == 4) {
      // Config: enter config mode
      configMode = true;
      menuActive = false;
      Serial.println(F("[CONFIG] Entered config mode"));
      return;
    }
    pidMode = mode;
    // Preset tunings for U1/U2 (example values; adjust as needed)
    if (mode == 0) {
      // AUTO: keep current tunings, ensure automatic, set to default setpoint
      manualMode = false;
      tempPID.SetMode(AUTOMATIC);
      setpointC = rtcDefaultSetpoint;
      Serial.println(F("PID Mode: AUTO"));
    } else if (mode == 1) {
      // MAN: manual mode
      manualMode = true;
      tempPID.SetMode(MANUAL);
      Serial.println(F("PID Mode: MANUAL"));
    } else if (mode == 2) {
      // U1 preset: load persisted RTC values (fallback to defaults if not set)
      Kp = rtcU1Kp; Ki = rtcU1Ki; Kd = rtcU1Kd;
      tempPID.SetTunings(Kp, Ki, Kd);
      manualMode = false;
      tempPID.SetMode(AUTOMATIC);
      setpointC = rtcU1Sp;
      Serial.println(F("PID Mode: U1 (preset)") );
      // Optionally start a short full-power preheat when U1 is selected
      if (rtcU1PreheatEnabled) {
        preheatActive = true;
        preheatWhich = 1;
        pidOutputBeforePreheat = pidOutput;
        preheatEndMs = millis() + (unsigned long)rtcU1PreheatMs;
        pidOutput = PWM_MAX;
        applyOutput(pidOutput);
        Serial.printf("[PREHEAT] U1 preheat ON for %lums\n", rtcU1PreheatMs);
      }
    } else if (mode == 3) {
      // U2 preset: load persisted RTC values
      Kp = rtcU2Kp; Ki = rtcU2Ki; Kd = rtcU2Kd;
      tempPID.SetTunings(Kp, Ki, Kd);
      manualMode = false;
      tempPID.SetMode(AUTOMATIC);
      setpointC = rtcU2Sp;
      Serial.println(F("PID Mode: U2 (preset)") );
      // Optionally start a short full-power preheat when U2 is selected
      if (rtcU2PreheatEnabled) {
        preheatActive = true;
        preheatWhich = 2;
        pidOutputBeforePreheat = pidOutput;
        preheatEndMs = millis() + (unsigned long)rtcU2PreheatMs;
        pidOutput = PWM_MAX;
        applyOutput(pidOutput);
        Serial.printf("[PREHEAT] U2 preheat ON for %lums\n", rtcU2PreheatMs);
      }
    }
    // ensure PID controller is updated with new parameters
    tempPID.SetTunings(Kp, Ki, Kd);
    // close the menu after selection
    menuActive = false;
    Serial.printf("[MENU] Applied mode %d\n", mode);

    // Notify remote clients (BLE) of mode change if available
    const char *mstr = MENU_ITEMS[pidMode];
#if USE_BLE
    // NimBLE variant
    if (chMode) {
      std::string s(mstr);
      chMode->setValue(s);
      chMode->notify();
    }
    if (chModeRead) {
      std::string ms = std::to_string(pidMode);
      chModeRead->setValue(ms);
      chModeRead->notify();
    }
#endif
  }

  void printHelp() {
    Serial.println(F("ESPAtomizer commands:"));
    Serial.println(F(" S <temp>   - set setpoint (C)"));
    Serial.println(F(" P <kp,ki,kd> - set PID tunings"));
    Serial.println(F(" O <0-1023> - manual output"));
    Serial.println(F(" H - help"));
    Serial.println(F(" Q - preheat status; Q1/Q2 toggle; Q1ms/Q2ms set durations"));
  }

  // Print a one-shot compact snapshot of key runtime state to serial
  void printSnapshot() {
    Serial.printf("[SNAP] temp=%.2fC set=%.1fC out=%d pwmMax=%d\n", isnan(inputC) ? NAN : inputC, setpointC, (int)pidOutput, (int)PWM_MAX);
    Serial.printf("[SNAP] kp=%.3f ki=%.3f kd=%.3f mode=%s manual=%d power=%d\n", Kp, Ki, Kd, MENU_ITEMS[pidMode], manualMode?1:0, systemEnabled?1:0);
    #if USE_BAT
    Serial.printf("[SNAP] batV=%.2fV batPct=%d\n", batteryVoltage, batteryPercent);
    #else
    Serial.println(F("[SNAP] batV=null batPct=null"));
    #endif
    #if USE_MAX6675
    Serial.printf("[SNAP] tcConn=%d\n", tcConnected?1:0);
    #endif
  }

  // Battery helpers moved to battery.{h,cpp}

  void handleSerial() {
      if (!Serial || !Serial.available()) return;
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) return;
      char cmd = toupper(line.charAt(0));
      if (cmd == 'H') { printHelp(); }
      else if (cmd == 'S') {
        double v = line.substring(1).toFloat();
        if (v > 0) setpointC = v;
      } else if (cmd == 'O') {
        int v = atoi(line.substring(1).c_str()); pidOutput = constrain(v, 0, (int)PWM_MAX); applyOutput(pidOutput);
      } else if (cmd == 'T') {
  #if USE_MAX6675
        double raw = thermocouple.readCelsius();
        bool conn = !isnan(raw);
        Serial.printf("[TDBG] raw=%.3f C, connected=%d\n", raw, conn ? 1 : 0);
        Serial.printf("[TDBG] inputC=%.3f, tcConnected=%d\n", inputC, tcConnected?1:0);
  #else
        Serial.println(F("[TDBG] MAX6675 support disabled in build."));
  #endif
          }
          else if (cmd == 'X') {
            // Digital drive test command: X1 = set OUTPUT_PIN HIGH, X0 = set LOW, XT = toggle once
            if (line.length() < 2) {
              Serial.println(F("[XDBG] Usage: X1=HIGH X0=LOW XT=TOGGLE"));
            } else {
              char a = toupper(line.charAt(1));
              if (a == '1') {
                pinMode(OUTPUT_PIN, OUTPUT);
                digitalWrite(OUTPUT_PIN, HIGH);
                Serial.println(F("[XDBG] OUTPUT_PIN forced HIGH (digital)"));
              } else if (a == '0') {
                pinMode(OUTPUT_PIN, OUTPUT);
                digitalWrite(OUTPUT_PIN, LOW);
                Serial.println(F("[XDBG] OUTPUT_PIN forced LOW (digital)"));
              } else if (a == 'T') {
                pinMode(OUTPUT_PIN, OUTPUT);
                digitalWrite(OUTPUT_PIN, HIGH);
                Serial.println(F("[XDBG] OUTPUT_PIN toggled HIGH for 1s"));
                delay(1000);
                digitalWrite(OUTPUT_PIN, LOW);
                Serial.println(F("[XDBG] OUTPUT_PIN released LOW"));
              } else {
                    Serial.println(F("[XDBG] Unknown X command"));
                  }
                }
              }
          else if (cmd == 'Z') {
            serialStreamingEnabled = !serialStreamingEnabled;
            Serial.printf("[XDBG] serialStreamingEnabled=%d\n", serialStreamingEnabled?1:0);
          }
          else if (cmd == 'B') {
            // B = battery snapshot; BC = battery charge check (duration defaults to 20s)
            if (line.length() >= 2 && toupper(line.charAt(1)) == 'C') {
              unsigned long dur = 20000UL;
              // optional numeric duration in seconds after BC
              if (line.length() > 2) dur = (unsigned long)atoi(line.substring(2).c_str()) * 1000UL;
              batteryChargeCheck(dur, 2000UL);
            } else {
              printBatteryDebug();
            }
          }
          else if (cmd == 'V') {
            // ADC scan: print analogRead raw and scaled voltage for candidate pins
            int pins[] = { BAT_PIN, ENC_PIN_A, ENC_PIN_B, OLED_SDA, OLED_SCL, OUTPUT_PIN, 0, 1, 2, 16, 19, 20, 21, 22, 23 };
            int np = sizeof(pins)/sizeof(pins[0]);
            for (int i = 0; i < np; ++i) {
              int p = pins[i];
              int raw = analogRead(p);
              double v_adc = (raw / 4095.0) * 3.3;
              Serial.printf("[ADC] pin=%d raw=%d v_adc=%.3fV\n", p, raw, v_adc);
            }
          }
          else if (cmd == 'E') {
            // Encoder diagnostics: 'E' prints state, 'EZ' zeroes counters
              if (line.length() >= 2 && toupper(line.charAt(1)) == 'Z') {
                encoderZeroCounters();
                Serial.println(F("[ENC] counters zeroed"));
              } else {
                int a = digitalRead(ENC_PIN_A);
                int b = digitalRead(ENC_PIN_B);
                Serial.printf("[ENC] encTicks=%ld encHandled=%ld prevState=%u A=%d B=%d\n", (long)encoderGetTicks(), (long)encoderGetHandled(), (unsigned)encoderGetPrevState(), a, b);
              }
          }
          else if (cmd == 'P') {
            // one-shot snapshot print
            printSnapshot();
          }
          else if (cmd == 'Q') {
            // Q    - print preheat settings
            // Q1   - toggle U1 preheat
            // Q2   - toggle U2 preheat
            // Q1ms <ms> - set U1 preheat duration
              if (line.length() == 1) {
              Serial.printf("[PREHEAT] U1 enabled=%d ms=%lums, U2 enabled=%d ms=%lums\n", rtcU1PreheatEnabled?1:0, rtcU1PreheatMs, rtcU2PreheatEnabled?1:0, rtcU2PreheatMs);
            } else {
              char a = toupper(line.charAt(1));
              if (a == '1') {
                if (line.length() == 2) {
                  rtcU1PreheatEnabled = !rtcU1PreheatEnabled;
                  Serial.printf("[PREHEAT] rtcU1PreheatEnabled=%d\n", rtcU1PreheatEnabled?1:0);
                } else if (line.substring(2).startsWith("ms")) {
                  unsigned long v = (unsigned long)atoi(line.substring(4).c_str()); if (v>0) rtcU1PreheatMs = v;
                  Serial.printf("[PREHEAT] rtcU1PreheatMs=%lums\n", rtcU1PreheatMs);
                }
              } else if (a == '2') {
                if (line.length() == 2) {
                  rtcU2PreheatEnabled = !rtcU2PreheatEnabled;
                  Serial.printf("[PREHEAT] rtcU2PreheatEnabled=%d\n", rtcU2PreheatEnabled?1:0);
                } else if (line.substring(2).startsWith("ms")) {
                  unsigned long v = (unsigned long)atoi(line.substring(4).c_str()); if (v>0) rtcU2PreheatMs = v;
                  Serial.printf("[PREHEAT] rtcU2PreheatMs=%lums\n", rtcU2PreheatMs);
                }
              }
            }
          }
      }

#if USE_OLED
  void updateDisplay() {
    if (!displayAvailable) return;
    display.clearDisplay();

    // If config mode is active, show config screen
    if (configMode) {
      // Paginated config list: compute how many rows fit and draw a window
      const int pad = 6;
      const int itemH = 10;
      const int menuW = OLED_WIDTH - (pad * 2);
      const int maxRows = (OLED_HEIGHT - (pad * 2)) / itemH;
      int visibleRows = maxRows;
      if (visibleRows < 1) visibleRows = 1;

      // Determine start index so the selected item remains visible
      int startIdx = configIndex - (visibleRows / 2);
      if (startIdx < 0) startIdx = 0;
      if (startIdx + visibleRows > CONFIG_ITEM_COUNT) startIdx = max(0, CONFIG_ITEM_COUNT - visibleRows);

      const int menuH = (itemH * visibleRows) + (pad);
      const int mx = pad;
      const int my = (OLED_HEIGHT - menuH) / 2;
      display.drawRect(mx, my, menuW, menuH, SSD1306_WHITE);

      for (int v = 0; v < visibleRows; ++v) {
        int i = startIdx + v;
        int iy = my + 4 + v * itemH;
        if (i == configIndex) {
          display.fillRect(mx + 2, iy - 1, menuW - 4, itemH, SSD1306_WHITE);
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
          display.setTextColor(SSD1306_WHITE);
        }
        display.setTextSize(1);
        display.setCursor(mx + 6, iy);
        display.print(CONFIG_ITEMS[i]);
        // Right-side value hints
        int valx = mx + menuW - 2 - 6*8; // reserve up to 8 chars
        display.setCursor(valx, iy);
        switch (i) {
          case 0: { char buf[12]; dtostrf(rtcDefaultSetpoint, 0, 1, buf); display.print(buf); display.print(rtcTempUnitIsC?"C":"F"); break; }
          case 1: display.print(rtcTempUnitIsC?"C":"F"); break;
          case 2: display.print(rtcBleEnabled?"ON":"OFF"); break;
          case 3: display.print(BLE_NAME_PRESETS[rtcBleNameIndex]); break;
          case 4: { char buf[12]; snprintf(buf, sizeof(buf), "%lums", rtcBleAdvIntervalMs); display.print(buf); break; }
          case 5: display.print(rtcBleOledIndicator?"ON":"OFF"); break;
          case 6: display.print("--"); break;
          case 7: display.print("Back"); break;
          default: break;
        }
      }

      // Draw small up/down arrows if there are more items off-screen
      if (startIdx > 0) {
        // draw up arrow at top-right corner
        int ax = mx + menuW - 10;
        int ay = my + 2;
        display.fillTriangle(ax, ay+6, ax+4, ay+2, ax+8, ay+6, SSD1306_WHITE);
      }
      if (startIdx + visibleRows < CONFIG_ITEM_COUNT) {
        int ax = mx + menuW - 10;
        int ay = my + menuH - 6;
        // down arrow
        display.fillTriangle(ax, ay-6, ax+4, ay-2, ax+8, ay-6, SSD1306_WHITE);
      }

      display.setTextColor(SSD1306_WHITE);
      display.display();
      return;
    }

    // If menu is active, draw a centered menu overlay and return

    if (menuActive) {
      const int pad = 8;
      const int itemH = 10; // pixels per item
      const int menuW = OLED_WIDTH - (pad * 2);
      const int menuH = (itemH * MENU_ITEM_COUNT) + (pad);
      const int mx = pad;
      const int my = (OLED_HEIGHT - menuH) / 2;
      // background frame
      display.drawRect(mx, my, menuW, menuH, SSD1306_WHITE);
      for (int i = 0; i < MENU_ITEM_COUNT; ++i) {
        int iy = my + 4 + i * itemH;
        if (i == menuIndex) {
          // highlight
          display.fillRect(mx + 2, iy - 1, menuW - 4, itemH, SSD1306_WHITE);
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
          display.setTextColor(SSD1306_WHITE);
        }
        display.setTextSize(1);
        display.setCursor(mx + 6, iy);
        display.print(MENU_ITEMS[i]);
      }
      // restore text color
      display.setTextColor(SSD1306_WHITE);
      display.display();
      return;
    }
  

    // Top-left: Power / Mode (small)
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    if (systemEnabled) display.print("PWR:ON "); else display.print("PWR:OFF");
    if (manualMode) display.print("[MAN]"); else display.print("[AUTO]");
    // BLE connection indicator: show a single character when a BLE client is connected
    #if USE_BLE
    if (rtcBleOledIndicator) {
      unsigned long _now = millis();
      int bleConnected = 0;
      if (bleServer) bleConnected = (bleServer->getConnectedCount() > 0) ? 1 : 0;
      bool show = false;
      if (bleAnimExpireMs > _now) {
        // blink during animation window (toggle every 200ms)
        show = (((_now / 200UL) & 1UL) == 0UL);
      } else {
        show = (bleConnected != 0);
      }
      if (show) display.print(" B");
    }
    #endif

    // Battery numeric (top-right)
    display.setTextSize(1);
    char batbuf[16];
    // Prefer a freshly-computed percent from `batteryVoltage` when available so
    // the display reflects the most recent ADC reads even if `batteryPercent`
    // hasn't been updated yet by the background sampler.
    if (!isnan(batteryVoltage)) {
      // Compute percent using configured divider/limits
      double p = (batteryVoltage - BAT_MIN_V) / (BAT_MAX_V - BAT_MIN_V) * 100.0;
      if (p < 0) p = 0; if (p > 100) p = 100;
      int pct = (int)(p + 0.5);
      snprintf(batbuf, sizeof(batbuf), "BAT:%d%%", pct);
    } else if (batteryPercent >= 0) {
      snprintf(batbuf, sizeof(batbuf), "BAT:%d%%", batteryPercent);
    } else {
      snprintf(batbuf, sizeof(batbuf), "BAT:--");
    }
    int batTw = (int)strlen(batbuf) * 6;
    display.setCursor(OLED_WIDTH - batTw - 2, 0);
    display.print(batbuf);

    // Small uptime + last-BLE-event line (right-aligned under the battery indicator)
    display.setTextSize(1);
    char leb[48]; snprintf(leb, sizeof(leb), "U:%lus %s", millis() / 1000UL, lastBleEvent);
    int lebw = (int)strlen(leb) * 6;
    if (lebw + 2 < OLED_WIDTH) display.setCursor(OLED_WIDTH - lebw - 2, 10);
    else display.setCursor(0, 10);
    display.print(leb);

    // Compute output percent and bar geometry early so we can position temp/SP above it
    int outPct = 0;
    if (PWM_MAX > 0) outPct = (int)((pidOutput / (double)PWM_MAX) * 100.0 + 0.5);
    const int barH = 6;            // bar height
    const int barMargin = 3;       // left/right margins
    const int barX = barMargin;
    const int barW = OLED_WIDTH - (barMargin * 2);
    const int bottomSpacing = 1;   // spacing from bottom
    const int barY = OLED_HEIGHT - barH - bottomSpacing;

    // Place setpoint and temperature directly above the bar
    const int tempH = 16; // height for text size 2
    const int spH = 16;   // height for text size 2 (increase SP size)
    const int vGap = 6;   // vertical gap (px) between lines and the bar

    // Compute percentY early so we can avoid overlap with setpoint
    const int textH1_local = 8; // height for text size 1
    int percentY = barY - 1 - textH1_local;
    if (percentY < 0) percentY = 0;

    int spY = barY - vGap - spH;           // setpoint top just above bar with vGap
    // If setpoint would overlap the percent label, push it up
    if (spY + spH > percentY - 1) spY = percentY - 1 - spH;
    if (spY < 8) spY = 8;                  // avoid drawing into very top area

    int tempY = spY - vGap - tempH;        // temperature above setpoint with vGap
    tempY += 3;                            // nudge temperature down by 3 px
    if (tempY < 0) tempY = 0;

    // Temperature (size 2)
    display.setTextSize(2);
    display.setCursor(0, tempY);
    if (isnan(inputC)) {
      display.print("---");
    } else {
      float dispTemp = rtcTempUnitIsC ? inputC : (inputC * 9.0f/5.0f + 32.0f);
      char tbuf[10]; dtostrf(dispTemp, 0, 1, tbuf);
      display.print(tbuf);
    }
    display.print(rtcTempUnitIsC ? "C" : "F");

    // Setpoint (size 2) directly under temperature (hidden in MAN mode)
    if (!manualMode) {
      display.setTextSize(2);
      if (spY + spH > OLED_HEIGHT - 1) spY = OLED_HEIGHT - 1 - spH;
      display.setCursor(0, spY);
      float dispSp = rtcTempUnitIsC ? setpointC : (setpointC * 9.0f/5.0f + 32.0f);
      char sbuf[12]; dtostrf(dispSp, 0, 1, sbuf);
      display.print("SP:"); display.print(sbuf); display.print(rtcTempUnitIsC ? "C" : "F");
    }


    // Fill proportionally (use full bar height, left-to-right)
    int innerW = barW; // full width available for fill
    int fillW = (int)((innerW * outPct) / 100.0);
    if (fillW > 0) display.fillRect(barX, barY, fillW, barH, SSD1306_WHITE);

    // Output percentage label: 1 px above the bar on the right (right-aligned)
    display.setTextSize(1);
    char obuf[12]; snprintf(obuf, sizeof(obuf), "%d%%", outPct);
    int ow = (int)strlen(obuf) * 6;
    int percentX = OLED_WIDTH - ow - 2;
    display.setCursor(percentX, percentY);
    display.print(obuf);

    // Draw a small icon next to the percentage label if battery is low (left of percent)
    if (batteryLow) {
      int iconCx = percentX - 8;
      int iconCy = percentY + (textH1_local / 2);
      if (iconCx < 2) iconCx = 2;
      if (iconCy < 2) iconCy = 2;
      display.fillCircle(iconCx, iconCy, 2, SSD1306_WHITE);
    }

    display.display();
  }
#endif
#endif // USE_BLE (balance missing #if at top of file)
