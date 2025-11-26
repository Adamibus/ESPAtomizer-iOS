// Centralized configuration for ESPAtomizer
// Edit these values to change board/pin/feature configuration

#ifndef ESPATOMIZER_CONFIG_H
#define ESPATOMIZER_CONFIG_H

// Features
#ifndef USE_BLE
#define USE_BLE 1
#endif
#ifndef USE_WIFI
#define USE_WIFI 0
#endif
#ifndef USE_MAX6675
#define USE_MAX6675 1
#endif

// Initial power-on state (0 = off, 1 = on). Defineable at build time.
#ifndef INIT_POWER_ON
#define INIT_POWER_ON 0
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
// OLED SDA is connected to the schematic net labeled SDA -> GPIO2 per the
// latest schematic/gerber mapping.
#define OLED_SDA 2
#endif
#ifndef OLED_SCL
// OLED SCL is connected to the schematic net labeled SCL -> GPIO1 per the
// latest schematic/gerber mapping.
#define OLED_SCL 1
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

// MAX6675 (thermocouple) pin defaults
// These use the Dn aliases above so you can override either the Dn aliases
// or the MAX6675_* macros directly at build time (e.g. -DMAX6675_SCK_PIN=19
// or -DD10_PIN=19). The PCB routes MAX_SCK->GPIO18, MAX_CS->GPIO20, MAX_SO->GPIO19.
#ifndef MAX6675_SCK_PIN
#define MAX6675_SCK_PIN D10_PIN // default: D10 (GPIO18)
#endif
#ifndef MAX6675_CS_PIN
#define MAX6675_CS_PIN D9_PIN  // default: D9  (GPIO20)
#endif
#ifndef MAX6675_SO_PIN
#define MAX6675_SO_PIN D8_PIN  // default: D8  (GPIO19 / MISO)
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

// Serial
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 230400
#endif

// PWM / Output
#ifndef OUTPUT_PIN
// MOSFET gate (PWM output) routed to the schematic net MOSFET_GATE which
// is connected to the module pad mapped to GPIO0 in the current schematic.
#define OUTPUT_PIN 0
#endif
#ifndef PWM_FREQ
#define PWM_FREQ 200
#endif
#ifndef PWM_RES_BITS
#define PWM_RES_BITS 10
#endif
#ifndef RELAY_MODE
#define RELAY_MODE 0
#endif

// Encoder
#ifndef USE_ENCODER
#define USE_ENCODER 1
#endif
#ifndef ENC_PIN_A
// Encoder A/B/SW nets are wired to module pads that map to GPIO16/23/22
// respectively in the current schematic (see PCB gerber). Map them explicitly
// so firmware follows the schematic wiring.
#define ENC_PIN_A 16
#endif
#ifndef ENC_PIN_B
#define ENC_PIN_B 23
#endif
#ifndef ENC_PIN_SW
#define ENC_PIN_SW 22
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

// Battery
#ifndef USE_BAT
#define USE_BAT 1
#endif
#ifndef BAT_PIN
// Battery ADC net is mapped to GPIO21 (unchanged).
#define BAT_PIN 21
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

#ifndef SLEEP_ON_IDLE_MS
#define SLEEP_ON_IDLE_MS 60000UL
#endif

#endif // ESPATOMIZER_CONFIG_H
