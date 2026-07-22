#ifndef ESPATOM_MENU_MANAGER_H
#define ESPATOM_MENU_MANAGER_H

/**
 * ESPAtomizer Menu Manager
 * 
 * Encapsulates all menu state and navigation logic.
 * Replaces scattered globals: menuActive, menuIndex, configMode, configIndex, configEditing, etc.
 * 
 * Provides:
 * - State machine for menu navigation
 * - Configuration submenu handling
 * - Confirmation dialog management
 * - Clean API for input handlers (encoder, button)
 */

#include <Arduino.h>

class MenuManager {
public:
  // Menu item constants
  static const int MENU_AUTO = 0;
  static const int MENU_MANUAL = 1;
  static const int MENU_U1 = 2;
  static const int MENU_U2 = 3;
  static const int MENU_CONFIG = 4;
  static const int MENU_ITEM_COUNT = 5;
  
  // Config item constants
  static const int CONFIG_DEFAULT_SP = 0;
  static const int CONFIG_UNIT = 1;
  static const int CONFIG_BLE_ON = 2;
  static const int CONFIG_NAME = 3;
  static const int CONFIG_ADV_INT = 4;
  static const int CONFIG_OLED = 5;
  static const int CONFIG_SAVE = 6;
  static const int CONFIG_FACTORY_RESET = 7;
  static const int CONFIG_FORGET = 8;
  static const int CONFIG_BACK = 9;
  static const int CONFIG_ITEM_COUNT = 10;
  
  /**
   * Initialize menu manager
   */
  static void init();
  
  // ===== MAIN MENU NAVIGATION =====
  
  /**
   * Open main menu (encoder long-press)
   */
  static void openMainMenu();
  
  /**
   * Close main menu
   */
  static void closeMainMenu();
  
  /**
   * Is main menu active?
   */
  static bool isMainMenuActive();
  
  /**
   * Move to next menu item
   */
  static void nextMenuItem();
  
  /**
   * Move to previous menu item
   */
  static void prevMenuItem();
  
  /**
   * Get current main menu item index (0-4)
   */
  static int getCurrentMenuIndex();
  
  /**
   * Get current main menu item name
   */
  static const char* getCurrentMenuName();

  /**
   * Get a menu item name by index
   */
  static const char* getMenuItemName(int index);
  
  /**
   * Select current menu item (button press)
   * If it's a mode (AUTO/MAN/U1/U2), switch to that mode
   * If it's CONFIG, open config submenu
   */
  static void selectCurrentMenuItem();
  
  // ===== CONFIG SUBMENU =====
  
  /**
   * Is config submenu active?
   */
  static bool isConfigActive();
  
  /**
   * Get current config item index
   */
  static int getCurrentConfigIndex();
  
  /**
   * Get current config item name
   */
  static const char* getCurrentConfigName();
  
  /**
   * Move to next config item
   */
  static void nextConfigItem();
  
  /**
   * Move to previous config item
   */
  static void prevConfigItem();
  
  /**
   * Start editing current config item
   */
  static void startEditingConfig();
  
  /**
   * Stop editing current config item
   */
  static void stopEditingConfig();
  
  /**
   * Is currently editing a config value?
   */
  static bool isEditingConfig();
  
  /**
   * Exit config submenu (back to main menu)
   */
  static void exitConfig();
  
  // ===== CONFIRMATION DIALOG =====
  
  /**
   * Show confirmation dialog for extreme setpoint change
   * @param proposedSetpoint Proposed new setpoint
   * @param timeoutMs How long to wait for user confirmation (auto-cancel after)
   */
  static void showConfirmation(double proposedSetpoint, unsigned long timeoutMs);
  
  /**
   * Is confirmation dialog active?
   */
  static bool isConfirmationActive();
  
  /**
   * Get proposed setpoint from confirmation dialog
   */
  static double getConfirmationSetpoint();
  
  /**
   * Check if confirmation dialog has timed out
   */
  static bool hasConfirmationTimedOut();
  
  /**
   * User confirmed (button press)
   */
  static void confirmSelection();
  
  /**
   * User cancelled (encoder long-press or timeout)
   */
  static void cancelConfirmation();
  
  // ===== STATE GETTERS (for display/rendering) =====
  
  /**
   * Get PID mode from selected menu item
   * Returns: 0=AUTO, 1=MANUAL, 2=U1, 3=U2, -1=CONFIG (not a mode)
   */
  static int getSelectedPidMode();
  
  /**
   * Timestamp when confirmation dialog started
   */
  static unsigned long getConfirmationStartMs();
  
  /**
   * Reset all menu state to defaults
   */
  static void reset();
  
private:
  // Prevent instantiation
  MenuManager() = delete;
  
  // Internal state
  static bool mainMenuActive;
  static int mainMenuIndex;
  
  static bool configActive;
  static int configIndex;
  static bool configEditing;
  
  static bool confirmationActive;
  static double confirmationSetpoint;
  static unsigned long confirmationStartMs;
  static unsigned long confirmationTimeoutMs;
  
  static const char* MENU_ITEMS[];
  static const char* CONFIG_ITEMS[];
};

#endif // ESPATOM_MENU_MANAGER_H
