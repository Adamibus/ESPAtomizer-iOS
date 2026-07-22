#ifndef ESPATOM_PREFERENCES_MANAGER_H
#define ESPATOM_PREFERENCES_MANAGER_H

/**
 * ESPAtomizer Preferences Manager
 * 
 * Centralizes all NVS (Non-Volatile Storage) read/write operations.
 * Eliminates duplicate preference code scattered in BLE callbacks.
 * 
 * Features:
 * - Typed getters/setters with validation
 * - Preference caching to reduce EEPROM reads
 * - Automatic bounds checking & clamping
 * - Serial logging for debugging
 */

#include <Arduino.h>
#include <Preferences.h>

class PreferencesManager {
public:
  /**
   * Initialize preferences manager
   * Should be called once at boot
   */
  static void init();
  
  // ===== PID PARAMETERS =====
  
  /**
   * Set Kp and persist to NVS
   * @param value New Kp (will be clamped to [0.1 .. 100.0])
   * @param mode Which mode to save for (0=AUTO, 2=U1, 3=U2)
   */
  static void setKp(double value, int mode = 0);
  static double getKp(int mode = 0);
  
  static void setKi(double value, int mode = 0);
  static double getKi(int mode = 0);
  
  static void setKd(double value, int mode = 0);
  static double getKd(int mode = 0);
  
  /**
   * Set setpoint and persist (clamped to [ENC_MIN_C .. ENC_MAX_C])
   */
  static void setSetpoint(double valueC, int mode = 0);
  static double getSetpoint(int mode = 0);
  
  /**
   * Default setpoint (shown on boot)
   */
  static void setDefaultSetpoint(double valueC);
  static double getDefaultSetpoint();
  
  // ===== BLE CONFIGURATION =====
  
  /**
   * BLE enabled/disabled
   */
  static void setBleEnabled(bool enabled);
  static bool getBleEnabled();
  
  /**
   * BLE device name (index into BLE_NAME_PRESETS)
   */
  static void setBleName(int index);
  static int getBleName();
  
  /**
   * BLE advertising interval (milliseconds)
   */
  static void setBleAdvInterval(unsigned long ms);
  static unsigned long getBleAdvInterval();
  
  /**
   * BLE OLED indicator animation enabled/disabled
   */
  static void setBleOledIndicator(bool enabled);
  static bool getBleOledIndicator();
  
  // ===== TEMPERATURE UNIT =====
  
  /**
   * Temperature unit: true=Celsius, false=Fahrenheit
   */
  static void setTempUnitC(bool isCelsius);
  static bool getTempUnitC();
  
  // ===== PRESET SAVE/LOAD =====
  
  /**
   * Save entire U1/U2 preset (Kp, Ki, Kd, Setpoint)
   * @param presetNum 1 for U1, 2 for U2
   */
  static void savePreset(int presetNum);
  
  /**
   * Load entire U1/U2 preset from NVS
   */
  static void loadPreset(int presetNum);
  
  // ===== UTILITY =====
  
  /**
   * Clear all preferences (factory reset)
   */
  static void clearAll();
  
  /**
   * Validate a double value against bounds
   * @return value clamped to [minV .. maxV], or NAN if input is invalid
   */
  static double validateDouble(const char* str, double minV, double maxV);
  
private:
  // Prevent instantiation (static-only class)
  PreferencesManager() = delete;
  
  // Internal NVS namespace
  static const char* PREF_NAMESPACE;
};

#endif // ESPATOM_PREFERENCES_MANAGER_H
