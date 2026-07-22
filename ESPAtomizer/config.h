// Centralized configuration for ESPAtomizer
// Edit these values to change board/pin/feature configuration
// Supports both XIAO ESP32-C3 and XIAO ESP32-C6 (interchangeable)

#ifndef ESPATOMIZER_CONFIG_H
#define ESPATOMIZER_CONFIG_H

// Board Selection (C3 and C6 are pin-compatible and use the same firmware)
// Set BOARD_TYPE at compile time: -DBOARD_TYPE=BOARD_XIAO_ESP32_C3 or -DBOARD_TYPE=BOARD_XIAO_ESP32_C6
// If not specified, firmware will auto-detect based on Arduino board selection
#ifndef BOARD_TYPE
  #if defined(ARDUINO_XIAO_ESP32C3)
    #define BOARD_TYPE BOARD_XIAO_ESP32_C3
  #elif defined(ARDUINO_XIAO_ESP32C6)
    #define BOARD_TYPE BOARD_XIAO_ESP32_C6
  #else
    // Default to C3 if not detected
    #define BOARD_TYPE BOARD_XIAO_ESP32_C3
  #endif
#endif

#define BOARD_XIAO_ESP32_C3 3
#define BOARD_XIAO_ESP32_C6 6

// Features
// BLE is ON by default: the iOS app talks to the device only over Bluetooth LE.
// Override with -DUSE_BLE=0 at build time if you want a BLE-less build.
#ifndef USE_BLE
#define USE_BLE 1
#endif

// Compile-time test mode: when set to 1, hardware I/O is stubbed and
// simulated values are used for temperature and battery to enable logic
// smoke-testing without a device.
#ifndef TEST_MODE
#define TEST_MODE 0
#endif

// Initial power-on state (0 = off, 1 = on). Defineable at build time.
#ifndef INIT_POWER_ON
#define INIT_POWER_ON 0
#endif

// Safety features (disable for development)
#ifndef ENABLE_WATCHDOG
#define ENABLE_WATCHDOG 1
#endif
#ifndef ENABLE_THERMAL_SAFETY
#define ENABLE_THERMAL_SAFETY 1
#endif
#ifndef ENABLE_RTC_PERSISTENCE
#define ENABLE_RTC_PERSISTENCE 1
#endif

// OLED
#ifndef USE_OLED
#define USE_OLED 1
#endif
#ifndef OLED_WIDTH
#define OLED_WIDTH 128
#endif
#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif
#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR 0x3C
#endif
#ifndef OLED_SDA
// Use board-defined I2C pins for portability across XIAO C3/C6
#define OLED_SDA SDA
#endif
#ifndef OLED_SCL
#define OLED_SCL SCL
#endif

// Convenience aliases for Seeed XIAO D-pin labels -> GPIO numbers
// These make it easier to reason about the module pin names used on the
// silkscreen (D0..D10) versus the underlying GPIO numbers. You may override
// any of these at build time (e.g. -DD10_PIN=19) or by defining them before
// including `config.h` in another file.
#ifndef D8_PIN
#define D8_PIN 19  // labeled D8 on XIAO module (GPIO19)
#endif
#ifndef D9_PIN
#define D9_PIN 20  // labeled D9 on XIAO module (GPIO20 / MISO)
#endif
#ifndef D10_PIN
#define D10_PIN 18 // labeled D10 on XIAO module (GPIO18 / MOSI)
#endif

// Composite module U2 (XIAO) left-row pad -> GPIO mapping (footprint-accurate)
// Pad numbers 1..7 correspond to the left header row on the socket footprint.
// These are the physical GPIOs exposed by the module footprint (use these
// in firmware to target the physical pins regardless of schematic label order).
#ifndef U2_PAD1_GPIO
#define U2_PAD1_GPIO 0   // Pad1 -> GPIO0 (D0)
#endif
#ifndef U2_PAD2_GPIO
#define U2_PAD2_GPIO 1   // Pad2 -> GPIO1 (D1)
#endif
#ifndef U2_PAD3_GPIO
#define U2_PAD3_GPIO 2   // Pad3 -> GPIO2 (D2)
#endif
#ifndef U2_PAD4_GPIO
#define U2_PAD4_GPIO 21  // Pad4 -> GPIO21 (D3)
#endif
#ifndef U2_PAD5_GPIO
#define U2_PAD5_GPIO 22  // Pad5 -> GPIO22 (D4 / SDA)
#endif
#ifndef U2_PAD6_GPIO
#define U2_PAD6_GPIO 23  // Pad6 -> GPIO23 (D5 / SCL)
#endif
#ifndef U2_PAD7_GPIO
#define U2_PAD7_GPIO 16  // Pad7 -> GPIO16 (D6 / PWM OUT)
#endif

// LEFT_ROW_PIN_* aliases map the left header row pads on U2 (footprint)
// directly to their footprint GPIO numbers. The schematic and footprint are
// now aligned; these aliases are the canonical mapping used by the firmware.
#define LEFT_ROW_PIN_1 U2_PAD1_GPIO
#define LEFT_ROW_PIN_2 U2_PAD2_GPIO
#define LEFT_ROW_PIN_3 U2_PAD3_GPIO
#define LEFT_ROW_PIN_4 U2_PAD4_GPIO
#define LEFT_ROW_PIN_5 U2_PAD5_GPIO
#define LEFT_ROW_PIN_6 U2_PAD6_GPIO
#define LEFT_ROW_PIN_7 U2_PAD7_GPIO

// Status LED. The XIAO ESP32-C6 defines LED_BUILTIN (onboard user LED); the XIAO ESP32-C3 has
// no user LED, so the core leaves LED_BUILTIN undefined and the status-blink code must be
// disabled. STATUS_LED_PIN < 0 disables it; override at build time if you wire an external LED
// (e.g. -DSTATUS_LED_PIN=10).
#ifndef STATUS_LED_PIN
  #ifdef LED_BUILTIN
    #define STATUS_LED_PIN LED_BUILTIN
  #else
    #define STATUS_LED_PIN -1
  #endif
#endif

// Serial
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 230400
#endif

// PWM / Output
#ifndef OUTPUT_PIN
// MOSFET gate (PWM output). Use the footprint-mapped left-row pin 7 (U2 pad7 -> GPIO16)
// This keeps the output tied to the physical socket mapping and avoids boot/strap pins.
#define OUTPUT_PIN LEFT_ROW_PIN_7
#endif
#ifndef PWM_FREQ
#define PWM_FREQ 200
#endif
#ifndef PWM_RES_BITS
#define PWM_RES_BITS 10
#endif
#ifndef PWM_MAX
#define PWM_MAX ((1U << PWM_RES_BITS) - 1U)
#endif
#ifndef RELAY_MODE
#define RELAY_MODE 0
#endif

// Encoder
#ifndef USE_ENCODER
#define USE_ENCODER 1
#endif
#ifndef ENC_PIN_A
// Prefer D1/D2/D3 for encoder to avoid boot/strap pins and UART TX
#define ENC_PIN_A D1
#endif
#ifndef ENC_PIN_B
#define ENC_PIN_B D2
#endif
#ifndef ENC_PIN_SW
#define ENC_PIN_SW D3
#endif
#ifndef ENC_EDGES_PER_DETENT
#define ENC_EDGES_PER_DETENT 2
#endif
#ifndef ENC_STEP_C
#define ENC_STEP_C 5.0
#endif
#ifndef ENC_DIR
#define ENC_DIR -1
#endif
#ifndef ENC_MENU_STEP_EDGES
#define ENC_MENU_STEP_EDGES 4
#endif
#ifndef ENC_MAN_STEP_PCT
#define ENC_MAN_STEP_PCT 5
#endif
#ifndef ENC_STEP_SCALE
#define ENC_STEP_SCALE 1.0
#endif
#ifndef ENC_EXTREME_DELTA_C
#define ENC_EXTREME_DELTA_C 100.0  // Extreme change: require confirmation for >100C jumps
#endif
#ifndef ENC_CONFIRM_TIMEOUT_MS
#define ENC_CONFIRM_TIMEOUT_MS 5000UL  // Auto-cancel confirmation after 5 seconds
#endif
#ifndef ENC_RATE_LIMIT_MS
#define ENC_RATE_LIMIT_MS 50  // Limit encoder input to one step per 50ms
#endif

// Battery
#ifndef USE_BAT
#define USE_BAT 1
#endif
#ifndef BAT_PIN
// Use analog pin macro for portability. A0 maps to ADC1 on both C3 and C6.
#define BAT_PIN A0
#endif
#ifndef BAT_R1
#define BAT_R1 100000.0
#endif
#ifndef BAT_R2
#define BAT_R2 100000.0
#endif
#ifndef BAT_SAMPLES
#define BAT_SAMPLES 8
#endif
#ifndef BAT_MIN_V
#define BAT_MIN_V 3.30
#endif
#ifndef BAT_MAX_V
#define BAT_MAX_V 4.20
#endif
#ifndef BAT_CUTOFF_V
#define BAT_CUTOFF_V 3.30
#endif
#ifndef BAT_LOW_WARNING_V
#define BAT_LOW_WARNING_V 3.50
#endif
#ifndef BAT_HYSTERESIS_V
#define BAT_HYSTERESIS_V 0.20
#endif
#ifndef CHARGER_REMOVED_WINDOW_MS
#define CHARGER_REMOVED_WINDOW_MS 5000UL
#endif
#ifndef CHARGER_RAMP_DOWN_STEPS
#define CHARGER_RAMP_DOWN_STEPS 10
#endif
#ifndef MAX_CONTINUOUS_RUN_MS
#define MAX_CONTINUOUS_RUN_MS 300000UL
#endif
#ifndef MAX_PWM_RAMP_RATE
#define MAX_PWM_RAMP_RATE 50.0
#endif
#ifndef THERMAL_RUNAWAY_TEMP_C
#define THERMAL_RUNAWAY_TEMP_C 320.0
#endif
#ifndef THERMAL_RUNAWAY_MARGIN_C
#define THERMAL_RUNAWAY_MARGIN_C 50.0  // Margin above setpoint before thermal runaway fault
#endif
#ifndef MAX_PWM_THRESHOLD
#define MAX_PWM_THRESHOLD 921  // ~90% of 1023 (PWM_MAX)
#endif
#ifndef MAX_ON_TIME_MS
#define MAX_ON_TIME_MS 30000UL  // Max 30 seconds at >90% output before thermal runaway check
#endif
#ifndef WATCHDOG_TIMEOUT_MS
#define WATCHDOG_TIMEOUT_MS 5000UL
#endif
#ifndef PID_TIMEOUT_MS
#define PID_TIMEOUT_MS 10000UL
#endif
#ifndef ABSOLUTE_MIN_TEMP_C
#define ABSOLUTE_MIN_TEMP_C 0.0
#endif
#ifndef ABSOLUTE_MAX_TEMP_C
#define ABSOLUTE_MAX_TEMP_C 350.0
#endif
#ifndef SENSOR_VALID_RECOVERY
#define SENSOR_VALID_RECOVERY 3
#endif
#ifndef SENSOR_FAULT_THRESHOLD
#define SENSOR_FAULT_THRESHOLD 5
#endif
#ifndef CHARGER_REMOVED_THRESHOLD_MV
#define CHARGER_REMOVED_THRESHOLD_MV 500  // 500mV drop indicates charger removed
#endif

// Allow booting when battery is absent or below cutoff if USB power is available.
// This makes USB fallback always enabled so low battery readings do not block
// startup on powered USB connections.
#ifndef ALLOW_BOOT_WITHOUT_BATTERY
#define ALLOW_BOOT_WITHOUT_BATTERY 1
#endif

// Define `VBUS_SENSE_PIN` to a GPIO that's wired to sense USB VBUS (HIGH when
// USB/5V present). Leave undefined if board has no sense pin.
// #define VBUS_SENSE_PIN D4

// When waiting for USB VBUS presence to permit boot without battery, give up
// after this many milliseconds and continue booting with a warning. Set to 0
// to wait indefinitely (not recommended). Default: 10s.
#ifndef VBUS_WAIT_MS
#define VBUS_WAIT_MS 10000UL
#endif

// External ADS1115 fallback configuration
// If you have an external ADS1115 library installed (e.g. Adafruit or a
// different driver by Maximiliano Ramirez), enable fallback to use it when
// the built-in driver fails to initialize. Adjust the header and type to
// match the installed library if necessary.
#ifndef EXTERNAL_ADS1115_FALLBACK
#define EXTERNAL_ADS1115_FALLBACK 1
#endif

// Default header/type assume Adafruit ADS1X15 library. If you use another
// library, override these macros (e.g. in a board-specific config file).
#ifndef EXTERNAL_ADS1115_HEADER
#define EXTERNAL_ADS1115_HEADER <Adafruit_ADS1X15.h>
#endif
#ifndef EXTERNAL_ADS1115_TYPE
#define EXTERNAL_ADS1115_TYPE Adafruit_ADS1115
#endif

// Buttons and timing
#ifndef BUTTON_ACTIVE_LOW
#define BUTTON_ACTIVE_LOW 1
#endif
#ifndef BUTTON_PIN
// Legacy sketch references `BUTTON_PIN`. Map it to the encoder switch pin so
// existing code continues to work; you may override at build-time if needed.
#define BUTTON_PIN ENC_PIN_SW
#endif
#ifndef BUTTON_DEBOUNCE_MS
#define BUTTON_DEBOUNCE_MS 50
#endif
#ifndef BUTTON_LONG_MS
#define BUTTON_LONG_MS 800
#endif

// ADS1115 Thermocouple Amplifier (I2C-based, 3.3V native)
#ifndef USE_ADS1115
#define USE_ADS1115 1         // Enable ADS1115 16-bit I2C thermocouple converter (K-type)
#endif
#ifndef ADS1115_I2C_ADDR
#define ADS1115_I2C_ADDR 0x48 // I2C address (A0 pin tied to GND = 0x48, or change to 0x49/0x4A/0x4B if needed)
#endif
#ifndef ADS1115_CHANNEL
#define ADS1115_CHANNEL 0     // ADC channel 0 for thermocouple input
#endif
#ifndef ADS1115_SAMPLES
#define ADS1115_SAMPLES 16    // Number of samples to average for noise reduction
#endif
#ifndef ADS1115_GAIN
#define ADS1115_GAIN GAIN_ONE // ±4.096V range (appropriate for K-type thermocouple conversion)
#endif

#ifndef SLEEP_ON_IDLE_MS
#define SLEEP_ON_IDLE_MS 60000UL
#endif

// Cooldown period after thermal runaway or safety shutdown
#ifndef COOLDOWN_REQUIRED_MS
#define COOLDOWN_REQUIRED_MS 60000UL  // 1 minute cooldown period
#endif

// Compile-time configuration sanity checks
#ifdef __cplusplus
static_assert(BAT_MIN_V < BAT_MAX_V, "BAT_MIN_V must be less than BAT_MAX_V");
static_assert(BAT_CUTOFF_V >= BAT_MIN_V && BAT_CUTOFF_V <= BAT_MAX_V, "BAT_CUTOFF_V must be within [BAT_MIN_V, BAT_MAX_V]");
static_assert(PWM_FREQ > 0, "PWM_FREQ must be > 0");
static_assert(PWM_RES_BITS >= 1 && PWM_RES_BITS <= 16, "PWM_RES_BITS must be between 1 and 16");
static_assert(ENC_EDGES_PER_DETENT > 0, "ENC_EDGES_PER_DETENT must be > 0");
static_assert(ENC_STEP_C > 0, "ENC_STEP_C must be > 0");
static_assert(SLEEP_ON_IDLE_MS >= 0, "SLEEP_ON_IDLE_MS must be non-negative");
#endif

#endif // ESPATOMIZER_CONFIG_H
