#ifndef ESPATOM_UI_MANAGER_H
#define ESPATOM_UI_MANAGER_H

/**
 * ESPAtomizer UI Manager
 * 
 * Encapsulates all display/UI rendering:
 * - OLED display initialization and management
 * - Display state queries (for rendering logic)
 * - Menu rendering (main menu, config, confirmation)
 * - Main display rendering (temperature, setpoint, PWM)
 * 
 * Uses MenuManager to query menu state
 * Uses getDisplayState() to query what to show
 */

#include "config.h"
#include <Arduino.h>
#include "StateManager.h"

#if USE_OLED

class UIManager {
public:
  /**
   * Initialize UI subsystem (OLED display)
   * Should be called once during setup()
   */
  static void init(GlobalState& gState);
  
  /**
   * Update display (throttled from loop)
   * Renders appropriate screen based on current state:
   * - Main display (temp, setpoint, PWM)
   * - Main menu (AUTO/MAN/U1/U2/Config)
   * - Config submenu
   * - Confirmation dialog
   */
  static void update();
  
  /**
   * Display state for rendering
   * Query what to display without holding display state
   */
  struct DisplayInfo {
    const char* mainMode;        // "AUTO", "MANUAL", "U1", "U2", "CONFIG"
    double temperature;
    double setpoint;
    int pwmDuty;                 // 0-1023
    int batteryPercent;
    bool bleConnected;
    bool menuActive;
    bool configActive;
    bool confirmationActive;
    const char* confirmationText;
    bool faultDetected;
    const char* faultReason;
  };
  
  /**
   * Get current display state for rendering
   * Caller can use this to understand what to show
   */
  static DisplayInfo getDisplayState();
  
  /**
   * Force immediate display refresh
   */
  static void forceRefresh();
  
  /**
   * Check if display is available
   */
  static bool isAvailable();
  
  /**
   * Get display failure count
   */
  static int getFailureCount();
  
private:
  UIManager() = delete;
  static GlobalState* pGState;
  
  // Render functions (internal)
  static void renderMainDisplay();
  static void renderMainMenu();
  static void renderConfigMenu();
  static void renderConfirmationDialog();
  static void renderFaultScreen();
};

#else

// Stub when OLED disabled
class UIManager {
public:
  struct DisplayInfo {
    const char* mainMode = "OFF";
    double temperature = 0;
    double setpoint = 0;
    int pwmDuty = 0;
    int batteryPercent = 0;
    bool bleConnected = false;
    bool menuActive = false;
    bool configActive = false;
    bool confirmationActive = false;
    const char* confirmationText = "";
    bool faultDetected = false;
    const char* faultReason = "";
  };
  
  static void init(GlobalState& gState) {}
  static void update() {}
  static DisplayInfo getDisplayState() { return DisplayInfo(); }
  static void forceRefresh() {}
  static bool isAvailable() { return false; }
  static int getFailureCount() { return 0; }
};

#endif  // USE_OLED

#endif  // ESPATOM_UI_MANAGER_H
