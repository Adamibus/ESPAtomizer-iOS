#include "CLIManager.h"
#include "PreferencesManager.h"
#include "SafetyManager.h"
#include "config.h"

GlobalState* CLIManager::pGState = nullptr;
static String serBuffer = "";

void CLIManager::init(GlobalState& gState) {
  pGState = &gState;
  Serial.println("[CLIManager] Initialized");
  printHelp();
}

void CLIManager::update() {
  if (!pGState) return;
  
  // Read available serial data
  while (Serial.available()) {
    char ch = Serial.read();
    
    if (ch == '\n' || ch == '\r') {
      if (serBuffer.length() > 0) {
        // Parse command: first char is command, rest are arguments
        char cmd = serBuffer.charAt(0);
        String args = serBuffer.substring(1);
        args.trim();
        
        handleCommand(cmd, args);
        serBuffer = "";
      }
    } else {
      serBuffer += ch;
    }
  }
}

void CLIManager::handleCommand(char cmd, const String& args) {
  if (!pGState) return;
  
  cmd = toupper(cmd);
  
  Serial.printf("\n[CLI] Command: %c %s\n", cmd, args.c_str());
  
  switch (cmd) {
    case 'S': cmd_setKp(args); break;
    case 'O': cmd_setKi(args); break;
    case 'T': cmd_setKd(args); break;
    case 'X': cmd_setSetpoint(args); break;
    case 'Z': cmd_toggleStreaming(args); break;
    case 'U': cmd_setOutput(args); break;
    case 'M': cmd_setMode(args); break;
    case 'R': 
      Serial.println(F("System reset"));
      ESP.restart();
      break;
    case 'D': 
      printDiagnostics();
      break;
    case 'V': 
      printStatus();
      break;
    case 'P': 
      Serial.println(F("[Preferences saved]"));
      break;
    case 'W': 
      Serial.println(F("[WiFi info]"));
      break;
    case 'Y': 
      Serial.println(F("[Bonds cleared]"));
      break;
    case 'Q': 
      Serial.println(F("[Diagnostics reset]"));
      pGState->diagnostics = DiagnosticsState();
      break;
    case 'B': 
      Serial.println(F("[Boot info]"));
      break;
    case 'E': 
      Serial.println(F("[Error log cleared]"));
      pGState->errorLog = ErrorLogState();
      break;
    case 'H':
    case '?':
      printHelp();
      break;
    default:
      Serial.printf("Unknown command: %c\n", cmd);
      printHelp();
  }
}

void CLIManager::cmd_setKp(const String& args) {
  if (args.length() == 0) {
    Serial.printf("Current Kp: %.3f\n", pGState->pidController.Kp);
    return;
  }
  double newKp = PreferencesManager::validateDouble(args.c_str(), 0.1, 100.0);
  if (!isnan(newKp)) {
    pGState->pidController.Kp = newKp;
    PreferencesManager::setKp(newKp, pGState->pidController.pidMode);
    Serial.printf("Kp set to %.3f\n", newKp);
  } else {
    Serial.println("ERROR: Invalid Kp value");
  }
}

void CLIManager::cmd_setKi(const String& args) {
  if (args.length() == 0) {
    Serial.printf("Current Ki: %.3f\n", pGState->pidController.Ki);
    return;
  }
  double newKi = PreferencesManager::validateDouble(args.c_str(), 0.01, 10.0);
  if (!isnan(newKi)) {
    pGState->pidController.Ki = newKi;
    PreferencesManager::setKi(newKi, pGState->pidController.pidMode);
    Serial.printf("Ki set to %.3f\n", newKi);
  } else {
    Serial.println("ERROR: Invalid Ki value");
  }
}

void CLIManager::cmd_setKd(const String& args) {
  if (args.length() == 0) {
    Serial.printf("Current Kd: %.3f\n", pGState->pidController.Kd);
    return;
  }
  double newKd = PreferencesManager::validateDouble(args.c_str(), 0.1, 1000.0);
  if (!isnan(newKd)) {
    pGState->pidController.Kd = newKd;
    PreferencesManager::setKd(newKd, pGState->pidController.pidMode);
    Serial.printf("Kd set to %.3f\n", newKd);
  } else {
    Serial.println("ERROR: Invalid Kd value");
  }
}

void CLIManager::cmd_setSetpoint(const String& args) {
  if (args.length() == 0) {
    Serial.printf("Current setpoint: %.1fC\n", pGState->pidController.setpointC);
    return;
  }
  #ifndef ENC_MIN_C
  #define ENC_MIN_C 50.0
  #endif
  #ifndef ENC_MAX_C
  #define ENC_MAX_C 300.0
  #endif
  
  double newSp = PreferencesManager::validateDouble(args.c_str(), (double)ENC_MIN_C, (double)ENC_MAX_C);
  if (!isnan(newSp)) {
    // Live setpoint only — no NVS write (hold-persists: a cold boot starts
    // from the device preset; see the BLE SETPOINT handler for the same rule).
    pGState->pidController.setpointC = newSp;
    Serial.printf("Setpoint set to %.1fC\n", newSp);
  } else {
    Serial.println("ERROR: Invalid setpoint value");
  }
}

void CLIManager::cmd_setOutput(const String& args) {
  if (args.length() == 0) {
    Serial.printf("Current output: %d (PWM_MAX=%d)\n", (int)pGState->pidController.pidOutput, PWM_MAX);
    return;
  }
  int newOut = constrain(atoi(args.c_str()), 0, (int)PWM_MAX);
  pGState->pidController.pidOutput = newOut;
  Serial.printf("Output set to %d\n", newOut);
}

void CLIManager::cmd_setMode(const String& args) {
  if (args.length() == 0) {
    const char* modes[] = { "AUTO", "MANUAL", "U1", "U2", "CONFIG" };
    Serial.printf("Current mode: %s (%d)\n", modes[pGState->pidController.pidMode], pGState->pidController.pidMode);
    return;
  }
  
  String mode = args;
  mode.toUpperCase();
  
  int newMode = -1;
  if (mode == "AUTO" || mode == "0") newMode = 0;
  else if (mode == "MANUAL" || mode == "1") newMode = 1;
  else if (mode == "U1" || mode == "2") newMode = 2;
  else if (mode == "U2" || mode == "3") newMode = 3;
  else if (mode == "CONFIG" || mode == "4") newMode = 4;
  
  if (newMode >= 0) {
    pGState->pidController.pidMode = newMode;
    Serial.printf("Mode set to %d\n", newMode);
  } else {
    Serial.println("ERROR: Invalid mode");
  }
}

void CLIManager::cmd_toggleStreaming(const String& args) {
  pGState->features.serialStreamingEnabled = !pGState->features.serialStreamingEnabled;
  Serial.printf("Serial streaming: %s\n", pGState->features.serialStreamingEnabled ? "ON" : "OFF");
}

void CLIManager::printHelp() {
  Serial.println(F("\n=== ESPAtomizer CLI Help ==="));
  Serial.println(F("S<value> - Set Kp (0.1-100.0)"));
  Serial.println(F("O<value> - Set Ki (0.01-10.0)"));
  Serial.println(F("T<value> - Set Kd (0.1-1000.0)"));
  Serial.println(F("X<value> - Set setpoint (50-300C)"));
  Serial.println(F("U<value> - Set output (0-1023)"));
  Serial.println(F("M<mode>  - Set mode (AUTO/MANUAL/U1/U2)"));
  Serial.println(F("Z        - Toggle serial streaming"));
  Serial.println(F("D        - Print diagnostics"));
  Serial.println(F("V        - Print status"));
  Serial.println(F("P        - Save preferences"));
  Serial.println(F("R        - Reset system"));
  Serial.println(F("H/?      - Print this help\n"));
}

void CLIManager::printStatus() {
  Serial.printf("\n=== System Status ===\n");
  Serial.printf("Mode: %d, Enabled: %d\n", pGState->pidController.pidMode, pGState->pidController.systemEnabled ? 1 : 0);
  Serial.printf("PID: Kp=%.3f Ki=%.3f Kd=%.3f\n", pGState->pidController.Kp, pGState->pidController.Ki, pGState->pidController.Kd);
  Serial.printf("Setpoint: %.1fC, Input: %.1fC, Output: %.0f\n", pGState->pidController.setpointC, pGState->pidController.inputC, pGState->pidController.pidOutput);
  Serial.printf("Battery: %.2fV (%d%%)\n", pGState->battery.voltage, pGState->battery.percent);
  Serial.printf("Safe: %s\n", SafetyManager::isSafe() ? "YES" : "NO");
  Serial.println();
}

void CLIManager::printDiagnostics() {
  Serial.printf("\n=== Diagnostics ===\n");
  Serial.printf("Loop iterations: %lu\n", pGState->diagnostics.loopIterations);
  Serial.printf("PID computations: %lu\n", pGState->diagnostics.pidComputations);
  Serial.printf("Output changes: %lu\n", pGState->diagnostics.outputChanges);
  Serial.printf("BLE writes: %lu\n", pGState->diagnostics.bleWrites);
  Serial.printf("Sensor reads: %lu (failed: %lu)\n", pGState->diagnostics.sensorReads, pGState->diagnostics.sensorReadsFailed);
  Serial.printf("Errors: %d\n", pGState->errorLog.totalErrors);
  Serial.println();
}

void CLIManager::setStreamingEnabled(bool enabled) {
  pGState->features.serialStreamingEnabled = enabled;
}
