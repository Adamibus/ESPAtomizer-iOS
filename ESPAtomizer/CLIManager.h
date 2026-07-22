#ifndef ESPATOM_CLI_MANAGER_H
#define ESPATOM_CLI_MANAGER_H

/**
 * ESPAtomizer CLI Manager
 * 
 * Encapsulates all serial command-line interface functionality.
 * Extracted from handleSerial() function (~500 lines).
 * 
 * Supports 15+ commands for:
 * - PID parameter tuning (S, O, T, X, Z)
 * - Mode switching (M, R)
 * - Diagnostics & debugging (D, V, P, W, Y, Q, B, E)
 * - Preferences (save, load, factory reset)
 */

#include <Arduino.h>
#include "StateManager.h"

class CLIManager {
public:
  /**
   * Initialize CLI subsystem
   * Should be called once during setup()
   */
  static void init(GlobalState& gState);
  
  /**
   * Handle serial input and process commands
   * Should be called from loop() regularly
   * 
   * Reads available serial bytes and processes commands
   */
  static void update();
  
  /**
   * Print help/command reference to serial
   */
  static void printHelp();
  
  /**
   * Print system diagnostics (called by 'D' command)
   */
  static void printDiagnostics();
  
  /**
   * Print current system status (called by 'V' command)
   */
  static void printStatus();
  
  /**
   * Enable/disable serial streaming output
   */
  static void setStreamingEnabled(bool enabled);
  
private:
  CLIManager() = delete;
  static GlobalState* pGState;
  
  // Command handlers
  static void handleCommand(char cmd, const String& args);
  static void cmd_setKp(const String& args);
  static void cmd_setKi(const String& args);
  static void cmd_setKd(const String& args);
  static void cmd_setSetpoint(const String& args);
  static void cmd_setOutput(const String& args);
  static void cmd_setMode(const String& args);
  static void cmd_toggleStreaming(const String& args);
};

#endif  // ESPATOM_CLI_MANAGER_H
