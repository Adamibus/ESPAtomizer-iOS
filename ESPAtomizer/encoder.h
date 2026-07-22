// encoder.h
#ifndef ENCODER_H
#define ENCODER_H

#include "config.h"
#include "StateManager.h"
#include <Arduino.h>

extern GlobalState gState;

// Encoder configuration defaults (moveable overrides should be placed in
// `encoder.h` rather than the main sketch to keep encoder tuning centralized.)
#ifndef USE_ENCODER
#define USE_ENCODER 1
#endif

// Lowered for better sensitivity to small/slow movements
#ifndef ENC_EDGES_PER_DETENT
#define ENC_EDGES_PER_DETENT 2
#endif

// Menu acceleration: window (ms) to increase accel and max accel multiplier
#ifndef ENC_MENU_ACCEL_WINDOW_MS
#define ENC_MENU_ACCEL_WINDOW_MS 200
#endif
#ifndef ENC_MENU_ACCEL_MAX
#define ENC_MENU_ACCEL_MAX 3
#endif

// Menu sensitivity: number of encoder edges required to produce one menu step
#ifndef ENC_MENU_STEP_EDGES
#define ENC_MENU_STEP_EDGES 4
#endif

// Default coarse step per detent (degrees C). You can change this for different base step sizes.
#ifndef ENC_STEP_C
#define ENC_STEP_C 5.0
#endif

// Encoder direction and sensitivity tweaks
// Set ENC_DIR to 1 for normal, -1 to invert rotation
#ifndef ENC_DIR
#define ENC_DIR 1
#endif
// Scale applied to the step size per detent to increase resolution (0 < scale <= 1)
#ifndef ENC_STEP_SCALE
#define ENC_STEP_SCALE 1.0
#endif

// Manual-mode encoder output step (% of PWM range per detent)
#ifndef ENC_MAN_STEP_PCT
#define ENC_MAN_STEP_PCT 5  // percent per detent
#endif

#ifndef ENC_MIN_C
#define ENC_MIN_C 100
#endif
#ifndef ENC_MAX_C
#define ENC_MAX_C 400
#endif

// ISR debounce (microseconds). Tuned to reduce contact bounce in mechanical encoders.
#ifndef ENC_ISR_DEBOUNCE_US
#define ENC_ISR_DEBOUNCE_US 800
#endif

// Inline/static implementation so the encoder logic can live in a single header
// and be included from the main sketch without separate .cpp file.

static volatile int32_t encTicks = 0;      // counts edges (A/B changes)
static int32_t encTicksHandled = 0; // last processed count
static volatile uint8_t encPrevState = 0;
static volatile unsigned long encLastIsrUs = 0;
// Runtime software inversion flag: when true, ISR will invert rotation direction
static volatile bool encSoftInvert = true;

// Small transition table for quadrature decoding. Index = (prev<<2)|curr
// Values: -1 = one step CCW, +1 = one step CW, 0 = no/invalid movement
static const int8_t ENC_TRANS_TABLE[16] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

static void IRAM_ATTR onEncChange() {
  unsigned long now = micros();
  // Debounce guard helps ignore mechanical bouncing without losing fast turns.
  // Tunable via `ENC_ISR_DEBOUNCE_US` (microseconds) defined in the sketch.
  if ((long)(now - encLastIsrUs) < (long)ENC_ISR_DEBOUNCE_US) return;
  encLastIsrUs = now;

  uint8_t a = (uint8_t)digitalRead(ENC_PIN_A) ? 1 : 0;
  uint8_t b = (uint8_t)digitalRead(ENC_PIN_B) ? 1 : 0;
  uint8_t state = (a << 1) | b; // 2-bit state

  uint8_t prev = encPrevState;
  encPrevState = state;

  uint8_t idx = (uint8_t)((prev << 2) | state);
  int8_t delta = ENC_TRANS_TABLE[idx & 0x0F];
  if (delta != 0) {
    int8_t mult = encSoftInvert ? -1 : 1;
    encTicks += (int32_t)delta * (int32_t)mult;
  }
}

static inline void encoderInit() {
  pinMode(ENC_PIN_A, INPUT_PULLUP);
  pinMode(ENC_PIN_B, INPUT_PULLUP);
  encPrevState = ((uint8_t)digitalRead(ENC_PIN_A) << 1) | (uint8_t)digitalRead(ENC_PIN_B);
  encLastIsrUs = 0;
  // Attach CHANGE interrupts to both channels. Many cores support interrupts
  // on these pins; if a platform doesn't, consider switching to polling.
  int ia = digitalPinToInterrupt(ENC_PIN_A);
  int ib = digitalPinToInterrupt(ENC_PIN_B);
  Serial.printf("[ENC] init: A_pin=%d B_pin=%d intA=%d intB=%d prevState=%u\n", ENC_PIN_A, ENC_PIN_B, ia, ib, (unsigned)encPrevState);
  if (ia == -1 || ib == -1) {
    Serial.println(F("[ENC] Warning: digitalPinToInterrupt returned -1 for one or both pins. Will fall back to using GPIO pin numbers for attachInterrupt()."));
  }
  if (ia != -1) attachInterrupt(ia, onEncChange, CHANGE);
  else attachInterrupt(ENC_PIN_A, onEncChange, CHANGE);
  if (ib != -1) attachInterrupt(ib, onEncChange, CHANGE);
  else attachInterrupt(ENC_PIN_B, onEncChange, CHANGE);
  Serial.println(F("[ENC] attachInterrupt() called for A and B (CHANGE)."));
  // Publish a short initialization summary for display on the boot screen
  snprintf(gState.input.lastEncoderInfo, 64, "A=%d B=%d ia=%d ib=%d inv=%d edges=%d db=%dus", ENC_PIN_A, ENC_PIN_B, ia, ib, encSoftInvert?1:0, ENC_EDGES_PER_DETENT, ENC_ISR_DEBOUNCE_US);
}

static inline void encoderSelfTest(unsigned long durationMs = 5000UL) {
  int ia = digitalPinToInterrupt(ENC_PIN_A);
  int ib = digitalPinToInterrupt(ENC_PIN_B);
  Serial.printf("[ENC] initial A=%d B=%d prevState=%u intA=%d intB=%d\n", digitalRead(ENC_PIN_A), digitalRead(ENC_PIN_B), (unsigned)encPrevState, ia, ib);
  if (ia == -1 || ib == -1) Serial.println(F("[ENC] Note: interrupt mapping indicates interrupts may not be available on one or both pins."));
  Serial.println(F("[ENC] Self-test: rotate encoder now for test period..."));
  unsigned long deadline = millis() + durationMs;
  int32_t last = encTicks;
  while (millis() < deadline) {
    int32_t cur = encTicks;
    if (cur != last) {
      Serial.printf("[ENC] tick delta=%ld total=%ld\n", (long)(cur - last), (long)cur);
      last = cur;
    }
    delay(50);
  }
  Serial.println(F("[ENC] Self-test complete."));
}

static inline int32_t encoderGetTicks() { return encTicks; }
static inline int32_t encoderGetHandled() { return encTicksHandled; }
static inline void encoderAddHandled(int32_t v) { encTicksHandled += v; }
static inline void encoderZeroCounters() { noInterrupts(); encTicks = 0; encTicksHandled = 0; interrupts(); }
static inline uint8_t encoderGetPrevState() { return encPrevState; }

#endif // ENCODER_H
