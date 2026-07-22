#include "UIManager.h"
#include "MenuManager.h"
#include "SafetyManager.h"
#include "oled.h"
#include "config.h"

#if USE_OLED

GlobalState* UIManager::pGState = nullptr;

void UIManager::init(GlobalState& gState) {
  pGState = &gState;
  
  Serial.println("[UIManager] Initializing OLED...");
  
  // OLED initialization (using existing tryInitOLED function)
  bool oledOk = tryInitOLED(OLED_SDA, OLED_SCL);
  
  if (oledOk) {
    pGState->display.available = true;
    pGState->display.display = &display;
    pGState->display.lastSuccessMs = millis();
    Serial.println("[UIManager] OLED initialized successfully");
  } else {
    pGState->display.available = false;
    Serial.println("[UIManager] OLED not available");
  }
}

void UIManager::update() {
  if (!pGState || !pGState->display.available) return;
  
  // Render based on current state
  if (SafetyManager::isSafe() == false) {
    renderFaultScreen();
  } else if (pGState->menu.active) {
    if (pGState->menu.configMode) {
      renderConfigMenu();
    } else {
      renderMainMenu();
    }
  } else if (pGState->menu.awaitingConfirmation) {
    renderConfirmationDialog();
  } else {
    renderMainDisplay();
  }
  
  pGState->display.lastSuccessMs = millis();
}

UIManager::DisplayInfo UIManager::getDisplayState() {
  DisplayInfo info;
  
  if (!pGState) return info;
  
  // Populate display info for caller to use
  switch (pGState->pidController.pidMode) {
    case 0: info.mainMode = "AUTO"; break;
    case 1: info.mainMode = "MANUAL"; break;
    case 2: info.mainMode = "U1"; break;
    case 3: info.mainMode = "U2"; break;
    case 4: info.mainMode = "CONFIG"; break;
    default: info.mainMode = "UNKNOWN";
  }
  
  info.temperature = pGState->pidController.inputC;
  info.setpoint = pGState->pidController.setpointC;
  info.pwmDuty = (int)pGState->pidController.pidOutput;
  info.batteryPercent = pGState->battery.percent;
  info.bleConnected = false; // Will be updated by BLEManager
  info.menuActive = pGState->menu.active;
  info.configActive = pGState->menu.configMode;
  info.confirmationActive = pGState->menu.awaitingConfirmation;
  info.faultDetected = !SafetyManager::isSafe();
  info.faultReason = SafetyManager::getLastFault();
  
  return info;
}

void UIManager::forceRefresh() {
  if (!pGState || !pGState->display.available) return;
  display.clearDisplay();
  display.display();
}

bool UIManager::isAvailable() {
  if (!pGState) return false;
  return pGState->display.available;
}

int UIManager::getFailureCount() {
  if (!pGState) return 0;
  return pGState->display.failureCount;
}

void UIManager::renderMainDisplay() {
  if (!pGState || !pGState->display.available) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // Mode line
  const char* modeStr = "?";
  switch (pGState->pidController.pidMode) {
    case 0: modeStr = "AUTO"; break;
    case 1: modeStr = "MAN"; break;
    case 2: modeStr = "U1"; break;
    case 3: modeStr = "U2"; break;
    case 4: modeStr = "CFG"; break;
  }
  display.printf("%s %s", pGState->pidController.systemEnabled ? "ON" : "OFF", modeStr);
  
  // Battery indicator
  if (pGState->battery.percent >= 0) {
    display.printf(" BAT:%d%%", pGState->battery.percent);
  }
  
  display.println();
  display.println();
  
  // Temperature display (large)
  display.setTextSize(2);
  display.printf("%.1f", pGState->pidController.inputC);
  display.setTextSize(1);
  display.println(pGState->menu.tempUnitIsC ? "C" : "F");
  
  // Setpoint
  display.printf("SP: %.1f%c\n", pGState->pidController.setpointC, pGState->menu.tempUnitIsC ? 'C' : 'F');
  
  // PWM output bar
  int barLength = (pGState->pidController.pidOutput / PWM_MAX) * 128;
  if (barLength > 128) barLength = 128;
  display.drawRect(0, OLED_HEIGHT - 8, 128, 8, SSD1306_WHITE);
  if (barLength > 0) {
    display.fillRect(1, OLED_HEIGHT - 7, barLength - 2, 6, SSD1306_WHITE);
  }
  display.printf("PWM:%d", (int)pGState->pidController.pidOutput);
  
  display.display();
}

void UIManager::renderMainMenu() {
  if (!pGState || !pGState->display.available) return;
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("MENU");
  
  // Menu items with cursor
  const char* items[] = { "AUTO", "MAN", "U1", "U2", "Config" };
  int idx = pGState->menu.index;
  
  display.setTextSize(1);
  for (int i = 0; i < 5; i++) {
    display.print(i == idx ? "> " : "  ");
    display.println(items[i]);
  }
  
  display.display();
}

void UIManager::renderConfigMenu() {
  if (!pGState || !pGState->display.available) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("CONFIG");
  
  // Config items with cursor
  int idx = pGState->menu.configIndex;
  const char* items[] = { "Default SP", "Unit", "BLE ON", "Name", "AdvInt", "OLED", "Save", "Factory", "Forget", "Back" };
  
  for (int i = 0; i < 10; i++) {
    display.print(i == idx ? "> " : "  ");
    display.println(items[i]);
  }
  
  display.display();
}

void UIManager::renderConfirmationDialog() {
  if (!pGState || !pGState->display.available) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("CONFIRM");
  display.println();
  
  double sp = pGState->menu.pendingSetpointC;
  display.printf("Change setpoint to");
  display.println();
  
  display.setTextSize(2);
  display.printf("%.0f%c ?", sp, pGState->menu.tempUnitIsC ? 'C' : 'F');
  display.setTextSize(1);
  display.println();
  
  display.println("Press button to confirm");
  display.println("or wait to cancel");
  
  display.display();
}

void UIManager::renderFaultScreen() {
  if (!pGState || !pGState->display.available) return;
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, OLED_HEIGHT / 2 - 8);
  
  display.println("FAULT!");
  
  display.setTextSize(1);
  display.print("Reason: ");
  display.println(SafetyManager::getLastFault());
  
  display.println("Heater disabled");
  display.println("Press reset to continue");
  
  display.display();
}

#else

// Stub implementations when OLED disabled
GlobalState* UIManager::pGState = nullptr;

void UIManager::init(GlobalState& gState) {
  pGState = &gState;
  Serial.println("[UI] OLED disabled at compile time");
}

#endif  // USE_OLED
