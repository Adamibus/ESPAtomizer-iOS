#include <Wire.h>
#if USE_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif



// Define some constants
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define PWM_MAX 255 // Maximum PWM value (adjust as needed)
#define BAT_MIN_V 3.7 // Minimum battery voltage (adjust as needed)
#define BAT_MAX_V 4.2 // Maximum battery voltage (adjust as needed)

// Define feature flags
#define USE_OLED 1       // Set to 0 to disable OLED support
#define USE_ENCODER 1    // Set to 0 to disable encoder support
#define USE_BAT 1        // Set to 0 to disable battery monitoring

#define PWM_PIN 3   // Example pin definitions, update as necessary
#define ENC_PIN_A 2
#define ENC_PIN_B 4
#define BUTTON_PIN 5
#define BAT_PIN A0

// Menu and config items
const int MENU_ITEM_COUNT = 5;
const char* MENU_ITEMS[MENU_ITEM_COUNT] = {
  "Power On/Off",
  "Mode: Auto",
  "Setpoint: 23C",
  "Preheat: 1s",
  "Config"
};

const int CONFIG_ITEM_COUNT = 10;
const char* CONFIG_ITEMS[CONFIG_ITEM_COUNT] = {
  "Default Setpoint:",
  "Temp Unit:",
  "BLE Enabled:",
  "BLE Name:",
  "BLE Adv Interval:",
  "BLE OLED Indicator:",
  "Save",
  "Factory Reset",
  "Forget BLE",
  "Back"
};

// System variables
volatile bool systemEnabled = false;
bool manualMode = false;
float setpointC = 23.0f; // Default setpoint in Celsius
float inputC = 0.0f;   // Current temperature reading
double pidOutput = 0.0;  // Current PID output value

// Error logging
struct ErrorLog {
  int totalErrors;
};
ErrorLog errorLog = {0};

// System health monitoring
struct SystemHealth {
  bool operational;
  char statusMessage[64];
};
SystemHealth systemHealth = {true, "OK"};

// OLED display object
#if USE_OLED
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool displayAvailable = false;
#endif

// Encoder handling (if enabled)
#if USE_ENCODER
int encoderPos = 0; // Current position of the encoder
volatile int lastEncoderPos = 0; // Last read position to detect changes
bool menuActive = false; // Menu is currently being displayed
int menuIndex = 0; // Current selected menu item index
#endif

// Configuration mode handling
bool configMode = false; // Config mode is active
int configIndex = 0; // Current selected configuration item index

// Battery monitoring (if enabled)
#if USE_BAT
double batteryVoltage = -1.0f; // Latest measured voltage
int batteryPercent = -1; // Derived percent from voltage
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  #if USE_OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    displayAvailable = false;
  } else {
    display.display();
    delay(500);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED Initialized");
    display.display();
    displayAvailable = true;
    delay(2000);
  }
  #endif

  // Initialize PWM output pin
  pinMode(PWM_PIN, OUTPUT);

  // Initialize encoder pins (if enabled)
  #if USE_ENCODER
  pinMode(ENC_PIN_A, INPUT_PULLUP);
  pinMode(ENC_PIN_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), readEncoder, CHANGE);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  #endif

  // Initialize battery monitoring (if enabled)
  #if USE_BAT
  analogReference(DEFAULT);
  pinMode(BAT_PIN, INPUT);
  #endif

  // Run initial self-tests
  runBootSelfTests();
}

void loop() {
  if (!systemHealth.operational) {
    handleFaults();
    delay(500);
    return;
  }

  // Read temperature sensor (adjust as needed)
  inputC = readTemperatureC();

  // Update PID output (adjust as needed)
  pidOutput = computePID(inputC, setpointC);

  // Apply output to heater
  applyOutput(pidOutput);

  // Handle serial commands
  handleSerialCommands();

  // Update display if available and not in menu mode
  #if USE_OLED
  updateDisplay();
  #endif

  // Check system health periodically (adjust as needed)
  static unsigned long lastHealthCheck = 0;
  unsigned long now = millis();
  if (now - lastHealthCheck > 10000) {
    checkSystemHealth();
    lastHealthCheck = now;
  }
}

// Function prototypes
void handleFaults();
float readTemperatureC();
double computePID(float currentTemp, float setpoint);
void applyOutput(double outputValue);
void handleSerialCommands();
#if USE_OLED
void updateDisplay();
#endif
void runBootSelfTests();
void checkSystemHealth();

// Fault handling routine
void handleFaults() {
  // Turn off system and display error message (adjust as needed)
  systemEnabled = false;
  applyOutput(0.0);
  #if USE_OLED
  if (displayAvailable) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("System Error");
    display.display();
  }
  #endif
}

// Read temperature from sensor (adjust as needed)
float readTemperatureC() {
  // Placeholder for actual temperature reading code
  // Return a dummy value or use a library to interface with your specific sensor
  return inputC + 0.1f; // Example: increase by 0.1C every loop iteration
}

// Compute PID output (adjust as needed)
double computePID(float currentTemp, float setpoint) {
  // Placeholder for actual PID computation code
  // Return a dummy value or implement your PID logic here
  if (currentTemp < setpoint) return 255.0; // Example: full power if below setpoint
  else return 0.0; // No power otherwise
}

// Apply output to heater (adjust as needed)
void applyOutput(double outputValue) {
  int pwmDuty = static_cast<int>(outputValue);
  analogWrite(PWM_PIN, pwmDuty); // Write PWM duty cycle to output pin
}

// Handle serial commands
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("power on")) {
      systemEnabled = true;
    } else if (command.equalsIgnoreCase("power off")) {
      systemEnabled = false;
      applyOutput(0.0); // Ensure heater is turned off
    } else if (command.startsWith("setpoint ")) {
      float newSetpoint = command.substring(9).toFloat();
      setpointC = newSetpoint;
    } else if (command.equalsIgnoreCase("menu")) {
      menuActive = true;
      menuIndex = 0; // Start at first item
    } else if (command.equalsIgnoreCase("config")) {
      configMode = true;
      configIndex = 0; // Start at first item
    } else {
      Serial.println("Unknown command");
    }
  }
}

#if USE_OLED
void updateDisplay() {
  if (!displayAvailable) return;
  display.clearDisplay();

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
        case 0: { char buf[12]; dtostrf(setpointC, 0, 1, buf); display.print(buf); break; }
        case 1: display.print("C"); break;
        case 2: display.print("--"); break;
        case 3: display.print("---"); break;
        case 4: display.print("-----"); break;
        case 5: display.print("--"); break;
        case 6: display.print("OK"); break;  // Save
        case 7: display.print("RST"); break; // Factory Reset
        case 8: display.print("---"); break; // Forget BLE
        case 9: display.print("<-"); break;  // Back
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

  // Top-left: Power / Mode (small)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  if (systemEnabled) display.print("PWR:ON "); else display.print("PWR:OFF");
  if (manualMode) display.print("[MAN]"); else display.print("[AUTO]");
  
  // System health indicators
  if (!systemHealth.operational) {
    display.print(" !ERR");
  } else if (errorLog.totalErrors > 0) {
    display.print(" E:");
    display.print(errorLog.totalErrors);
  }
  
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
    char tbuf[10]; dtostrf(inputC, 0, 1, tbuf);
    display.print(tbuf);
  }
  display.print("C");

  // Setpoint (size 2) directly under temperature (hidden in MAN mode)
  if (!manualMode) {
    display.setTextSize(2);
    if (spY + spH > OLED_HEIGHT - 1) spY = OLED_HEIGHT - 1 - spH;
    display.setCursor(0, spY);
    char sbuf[12]; dtostrf(setpointC, 0, 1, sbuf);
    display.print("SP:"); display.print(sbuf); display.print("C");
  }

  // Draw bar border (always visible)
  display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
  
  // Fill proportionally (use full bar height, left-to-right)
  int innerW = barW - 2; // account for border
  int fillW = (int)((innerW * outPct) / 100.0);
  if (fillW > 0) display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);
  
  // Add visual warning pattern if faults present
  if (sensorFaulted || thermalRunawayFaulted || watchdogFaulted) {
    // Draw diagonal warning stripes on bar
    for (int x = barX; x < barX + barW; x += 4) {
      display.drawLine(x, barY, x + 2, barY + barH - 1, SSD1306_WHITE);
    }
  }

  // Output percentage label: 1 px above the bar on the right (right-aligned)
  display.setTextSize(1);
  char obuf[12]; snprintf(obuf, sizeof(obuf), "%d%%", outPct);
  int ow = (int)strlen(obuf) * 6;
  int percentX = OLED_WIDTH - ow - 2;
  display.setCursor(percentX, percentY);
  display.print(obuf