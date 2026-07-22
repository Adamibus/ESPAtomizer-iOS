#ifndef ESPATOM_BLE_MANAGER_H
#define ESPATOM_BLE_MANAGER_H

/**
 * ESPAtomizer BLE Manager
 * 
 * Encapsulates all BLE-related functionality:
 * - BLE server initialization
 * - Characteristic callbacks (ChCallbacks class)
 * - BLE state management via GlobalState.ble
 * 
 * Uses PreferencesManager for all NVS operations.
 * Uses MenuManager for mode/config changes.
 */

#include "config.h"
#include <Arduino.h>
#include "StateManager.h"

#if USE_BLE

class BLEManager {
public:
  /**
   * Initialize BLE subsystem
   * Should be called once during setup() after PreferencesManager::init()
   * 
   * @param gState Reference to global state object
   */
  static void init(GlobalState& gState);
  
  /**
   * Handle BLE loop updates (if needed)
   */
  static void update();
  
  /**
   * Check if BLE client is connected
   */
  static bool isConnected();
  
  /**
   * Get last BLE event description (for display/debug)
   */
  static const char* getLastEvent();
  
  /**
   * Manually notify all characteristics (refresh client display)
   */
  static void notifyAllCharacteristics();
  
  /**
   * Clear all stored BLE bonds (factory reset)
   */
  static void clearBonds();
  
  /**
   * Change BLE device name and restart advertising
   */
  static void setDeviceName(int nameIndex);
  
private:
  BLEManager() = delete;
  static GlobalState* pGState;
  static bool isConnectedFlag;
};

#else

// Stub when BLE disabled
class BLEManager {
public:
  static void init(GlobalState& gState) {}
  static void update() {}
  static bool isConnected() { return false; }
  static const char* getLastEvent() { return "ble:off"; }
  static void notifyAllCharacteristics() {}
  static void clearBonds() {}
  static void setDeviceName(int nameIndex) {}
};

#endif  // USE_BLE

#endif  // ESPATOM_BLE_MANAGER_H
