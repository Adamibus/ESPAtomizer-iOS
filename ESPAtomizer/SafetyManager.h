#ifndef ESPATOM_SAFETY_MANAGER_H
#define ESPATOM_SAFETY_MANAGER_H

/**
 * ESPAtomizer Safety Manager
 * 
 * Encapsulates all safety-critical functionality:
 * - Watchdog timeout detection (loop stall, PID timeout)
 * - Thermal runaway protection (over-temp, margin, max-on-time)
 * - Battery safety (low voltage, charger removal, ramp-down)
 * - Sensor fault detection & recovery
 * 
 * All safety state managed via GlobalState::SafetyState struct
 */

#include <Arduino.h>
#include "StateManager.h"

class SafetyManager {
public:
  /**
   * Initialize safety subsystem
   * Should be called once during setup()
   */
  static void init(GlobalState& gState);
  
  /**
   * Update safety checks (should be called every loop iteration)
   * Checks watchdog, thermal runaway, battery, sensor faults
   * Sets fault flags in gState.safety
   */
  static void update();
  
  /**
   * Is system safe to operate heater?
   * Returns false if any safety fault detected
   */
  static bool isSafe();
  
  /**
   * Get description of last fault (if any)
   */
  static const char* getLastFault();
  
  /**
   * Reset all fault flags and fault timers
   */
  static void clearFaults();
  
  /**
   * Manually record watchdog loop entry timestamp
   * Should be called at top of loop()
   */
  static void markLoopEntry();
  
  /**
   * Manually record PID computation timestamp
   * Should be called after tempPID.Compute()
   */
  static void markPIDCompute();
  
  /**
   * Manually record sensor reading timestamp
   * Should be called after reading temperature
   */
  static void markSensorRead(double tempC, bool valid);
  
  /**
   * Get current sensor fault status
   */
  static bool getSensorFaulted();
  
  /**
   * Get current thermal runaway status
   */
  static bool getThermalRunaway();
  
  /**
   * Get current watchdog status
   */
  static bool getWatchdogFaulted();
  
private:
  SafetyManager() = delete;
  static GlobalState* pGState;
};

#endif  // ESPATOM_SAFETY_MANAGER_H
