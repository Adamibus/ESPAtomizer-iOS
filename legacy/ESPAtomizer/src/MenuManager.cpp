#include "MenuManager.h"

// Static member initializations
bool MenuManager::mainMenuActive = false;
int MenuManager::mainMenuIndex = 0;

bool MenuManager::configActive = false;
int MenuManager::configIndex = 0;
bool MenuManager::configEditing = false;

bool MenuManager::confirmationActive = false;
double MenuManager::confirmationSetpoint = 0.0;
unsigned long MenuManager::confirmationStartMs = 0;
unsigned long MenuManager::confirmationTimeoutMs = 0;

const char* MenuManager::MENU_ITEMS[] = { "AUTO", "MAN", "U1", "U2", "Config" };
const char* MenuManager::CONFIG_ITEMS[] = { "Default SP", "Unit", "BLE ON", "Name", "AdvInt", "OLED", "Save", "Factory Reset", "Forget", "Back" };

void MenuManager::init() {
  Serial.println("[MenuManager] Initialized");
}

const char* MenuManager::getMenuItemName(int index) {
  if (index >= 0 && index < MENU_ITEM_COUNT) {
    return MENU_ITEMS[index];
  }
  return "Unknown";
}

// ===== MAIN MENU NAVIGATION =====

void MenuManager::openMainMenu() {
  mainMenuActive = true;
  mainMenuIndex = 0;
  configActive = false;
  confirmationActive = false;
  Serial.println("[Menu] Main menu opened");
}

void MenuManager::closeMainMenu() {
  mainMenuActive = false;
  configActive = false;
  Serial.println("[Menu] Main menu closed");
}

bool MenuManager::isMainMenuActive() {
  return mainMenuActive;
}

void MenuManager::nextMenuItem() {
  if (mainMenuActive && !configActive) {
    mainMenuIndex = (mainMenuIndex + 1) % MENU_ITEM_COUNT;
    Serial.printf("[Menu] Next item: %d (%s)\n", mainMenuIndex, MENU_ITEMS[mainMenuIndex]);
  }
}

void MenuManager::prevMenuItem() {
  if (mainMenuActive && !configActive) {
    mainMenuIndex = (mainMenuIndex - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
    Serial.printf("[Menu] Prev item: %d (%s)\n", mainMenuIndex, MENU_ITEMS[mainMenuIndex]);
  }
}

int MenuManager::getCurrentMenuIndex() {
  return mainMenuIndex;
}

const char* MenuManager::getCurrentMenuName() {
  if (mainMenuIndex >= 0 && mainMenuIndex < MENU_ITEM_COUNT) {
    return MENU_ITEMS[mainMenuIndex];
  }
  return "ERROR";
}

void MenuManager::selectCurrentMenuItem() {
  if (mainMenuActive && !configActive && mainMenuIndex == MENU_CONFIG) {
    configActive = true;
    configIndex = 0;
    configEditing = false;
    Serial.println("[Menu] Config submenu opened");
  } else {
    Serial.printf("[Menu] Selected item %d: %s\n", mainMenuIndex, MENU_ITEMS[mainMenuIndex]);
  }
}

// ===== CONFIG SUBMENU =====

bool MenuManager::isConfigActive() {
  return configActive;
}

int MenuManager::getCurrentConfigIndex() {
  return configIndex;
}

const char* MenuManager::getCurrentConfigName() {
  if (configIndex >= 0 && configIndex < CONFIG_ITEM_COUNT) {
    return CONFIG_ITEMS[configIndex];
  }
  return "ERROR";
}

void MenuManager::nextConfigItem() {
  if (configActive) {
    configIndex = (configIndex + 1) % CONFIG_ITEM_COUNT;
    configEditing = false;
    Serial.printf("[Config] Next item: %d (%s)\n", configIndex, CONFIG_ITEMS[configIndex]);
  }
}

void MenuManager::prevConfigItem() {
  if (configActive) {
    configIndex = (configIndex - 1 + CONFIG_ITEM_COUNT) % CONFIG_ITEM_COUNT;
    configEditing = false;
    Serial.printf("[Config] Prev item: %d (%s)\n", configIndex, CONFIG_ITEMS[configIndex]);
  }
}

void MenuManager::startEditingConfig() {
  if (configActive) {
    configEditing = true;
    Serial.printf("[Config] Started editing: %s\n", CONFIG_ITEMS[configIndex]);
  }
}

void MenuManager::stopEditingConfig() {
  if (configActive && configEditing) {
    configEditing = false;
    Serial.printf("[Config] Stopped editing: %s\n", CONFIG_ITEMS[configIndex]);
  }
}

bool MenuManager::isEditingConfig() {
  return configEditing;
}

void MenuManager::exitConfig() {
  if (configActive) {
    configActive = false;
    configIndex = 0;
    configEditing = false;
    Serial.println("[Config] Exited config submenu");
  }
}

// ===== CONFIRMATION DIALOG =====

void MenuManager::showConfirmation(double proposedSetpoint, unsigned long timeoutMs) {
  confirmationActive = true;
  confirmationSetpoint = proposedSetpoint;
  confirmationStartMs = millis();
  confirmationTimeoutMs = timeoutMs;
  Serial.printf("[Confirm] Showing confirmation dialog: %.1fC (timeout %lu ms)\n", proposedSetpoint, timeoutMs);
}

bool MenuManager::isConfirmationActive() {
  return confirmationActive;
}

double MenuManager::getConfirmationSetpoint() {
  return confirmationSetpoint;
}

bool MenuManager::hasConfirmationTimedOut() {
  if (!confirmationActive) return false;
  
  unsigned long elapsed = millis() - confirmationStartMs;
  return elapsed >= confirmationTimeoutMs;
}

void MenuManager::confirmSelection() {
  if (confirmationActive) {
    Serial.printf("[Confirm] Confirmed setpoint: %.1fC\n", confirmationSetpoint);
    confirmationActive = false;
  }
}

void MenuManager::cancelConfirmation() {
  if (confirmationActive) {
    Serial.println("[Confirm] Confirmation cancelled");
    confirmationActive = false;
  }
}

// ===== STATE GETTERS =====

int MenuManager::getSelectedPidMode() {
  if (!mainMenuActive || configActive) return -1;
  
  switch (mainMenuIndex) {
    case MENU_AUTO: return 0;
    case MENU_MANUAL: return 1;
    case MENU_U1: return 2;
    case MENU_U2: return 3;
    case MENU_CONFIG: return -1;  // Not a mode
    default: return -1;
  }
}

unsigned long MenuManager::getConfirmationStartMs() {
  return confirmationStartMs;
}

void MenuManager::reset() {
  mainMenuActive = false;
  mainMenuIndex = 0;
  configActive = false;
  configIndex = 0;
  configEditing = false;
  confirmationActive = false;
  Serial.println("[Menu] State reset");
}
