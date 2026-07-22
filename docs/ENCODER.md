## Hardware

- Encoder: Standard mechanical incremental encoder with push switch (e.g., KY-040 or bare EC11).
- Connections (for this project):
  - A (CLK) → GPIO0 (D0)
  - B (DT)  → GPIO1 (D1)
  - SW (push) → GPIO2 (D2) with INPUT_PULLUP
  - C (common) → GND
  - Note: Bare EC11 encoders don't need VCC. If you use a KY‑040 module, connect its VCC to 3.3 V so its onboard pull‑ups work.
- Enable internal pull-ups on A and B (INPUT_PULLUP). Add 0.01–0.1 µF caps to GND on A/B if needed for extra debounce.

Suggested pins on XIAO ESP32‑C6 (this repo's defaults):
- Encoder A/B/SW: GPIO0 (A), GPIO1 (B), GPIO2 (SW)
- I2C OLED: SDA=GPIO22, SCL=GPIO23
- PWM output: GPIO16 (D6)
- MAX6675: SCK=GPIO19, CS=GPIO18, SO=GPIO20

## Firmware changes (summary)

- Disable the potentiometer path (set `USE_POT 0`) and add an encoder handler.
- Use interrupts to detect A/B state changes and accumulate a position delta.
- Map encoder position to setpoint range (e.g., 100–350 °C); support acceleration if desired.
- Continue to use the existing push button logic on GPIO16 (or wire encoder SW to GPIO16).

### Minimal code snippet

```cpp
// Build flags
#define USE_POT 0

// Encoder pins
#define ENC_A 0  // D0
#define ENC_B 1  // D1

volatile int encDelta = 0;

void IRAM_ATTR encISR() {
  int a = digitalRead(ENC_A);
  int b = digitalRead(ENC_B);
  // Gray decoding: A leads B for one direction
  static int last = 0;
  int cur = (a << 1) | b;
  int key = (last << 2) | cur;
  // 00->01->11->10->00 = +1; reverse = -1
  switch (key) {
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
      encDelta++;
      break;
    case 0b0010:
    case 0b0100:
    case 0b1101:
    case 0b1011:
      encDelta--;
      break;
  }
  last = cur;
}

void setupEncoder() {
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encISR, CHANGE);
}

void loopEncoderUpdate() {
  static int accum = 0;
  noInterrupts();
  int d = encDelta;
  encDelta = 0;
  interrupts();
  if (d != 0) {
    accum += d;
    // Every 2 detents adjust setpoint by 1 °C (tune to taste)
    while (accum >= 2) { setpointC += 1; accum -= 2; }
    while (accum <= -2) { setpointC -= 1; accum += 2; }
    if (setpointC < POT_MIN_C) setpointC = POT_MIN_C;
    if (setpointC > POT_MAX_C) setpointC = POT_MAX_C;
  }
}
```

Integrate:
- Call `setupEncoder()` in `setup()` (after Serial).
- Call `loopEncoderUpdate()` inside your main `loop()` before PID compute.
- Remove the potentiometer averaging block (`#if USE_POT`).

## Pros and cons

Pros:
- No ADC noise or end-stop deadband; repeatable digital steps
- Push button already present; encoder SW can share BTN pin
- Fine-grained control; easy to add acceleration

Cons:
- Requires two GPIOs instead of one
- Mechanical encoders need debounce; ISR + simple state machine handles it well

## BOM impact

- Encoder (EC11) is often similar cost to a decent potentiometer, sometimes cheaper
- Removes the need for precision resistor divider for ADC setpoint

## PCB notes

- Place encoder footprint on the board edge if using a panel-mount shaft
- Keep A/B traces short and pulled-up; route away from the heater loop
- Add mounting holes if using a panel encoder with nut/washer
