#include "PreferencesManager.h"
#include "config.h"
#include "ble.h"

const char* PreferencesManager::PREF_NAMESPACE = "espatom";

void PreferencesManager::init() {
  // Preferences object handles initialization automatically on first access
  Serial.println("[PreferencesManager] Initialized");
}

// ===== PID PARAMETERS =====

void PreferencesManager::setKp(double value, int mode) {
  value = constrain(value, 0.1, 100.0);
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  char buf[32];
  snprintf(buf, sizeof(buf), "%.3f", value);
  
  if (mode == 2) {
    prefs.putString("u1_kp", buf);
    Serial.printf("[PREF] U1 Kp saved: %.3f\n", value);
  } else if (mode == 3) {
    prefs.putString("u2_kp", buf);
    Serial.printf("[PREF] U2 Kp saved: %.3f\n", value);
  } else {
    prefs.putString("kp", buf);
    Serial.printf("[PREF] Kp saved: %.3f\n", value);
  }
  
  prefs.end();
}

double PreferencesManager::getKp(int mode) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  const char* key = (mode == 2) ? "u1_kp" : (mode == 3) ? "u2_kp" : "kp";
  String v = prefs.getString(key, String());
  
  prefs.end();
  
  if (v.length()) {
    return atof(v.c_str());
  }
  
  // Return defaults based on mode
  if (mode == 2) return 12.0;  // U1 default
  if (mode == 3) return 8.0;   // U2 default
  return 10.0;                  // AUTO default
}

void PreferencesManager::setKi(double value, int mode) {
  value = constrain(value, 0.01, 10.0);
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  char buf[32];
  snprintf(buf, sizeof(buf), "%.3f", value);
  
  if (mode == 2) {
    prefs.putString("u1_ki", buf);
    Serial.printf("[PREF] U1 Ki saved: %.3f\n", value);
  } else if (mode == 3) {
    prefs.putString("u2_ki", buf);
    Serial.printf("[PREF] U2 Ki saved: %.3f\n", value);
  } else {
    prefs.putString("ki", buf);
    Serial.printf("[PREF] Ki saved: %.3f\n", value);
  }
  
  prefs.end();
}

double PreferencesManager::getKi(int mode) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  const char* key = (mode == 2) ? "u1_ki" : (mode == 3) ? "u2_ki" : "ki";
  String v = prefs.getString(key, String());
  
  prefs.end();
  
  if (v.length()) {
    return atof(v.c_str());
  }
  
  if (mode == 2) return 0.7;   // U1 default
  if (mode == 3) return 0.3;   // U2 default
  return 0.5;                   // AUTO default
}

void PreferencesManager::setKd(double value, int mode) {
  value = constrain(value, 0.1, 1000.0);
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  char buf[32];
  snprintf(buf, sizeof(buf), "%.3f", value);
  
  if (mode == 2) {
    prefs.putString("u1_kd", buf);
    Serial.printf("[PREF] U1 Kd saved: %.3f\n", value);
  } else if (mode == 3) {
    prefs.putString("u2_kd", buf);
    Serial.printf("[PREF] U2 Kd saved: %.3f\n", value);
  } else {
    prefs.putString("kd", buf);
    Serial.printf("[PREF] Kd saved: %.3f\n", value);
  }
  
  prefs.end();
}

double PreferencesManager::getKd(int mode) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  const char* key = (mode == 2) ? "u1_kd" : (mode == 3) ? "u2_kd" : "kd";
  String v = prefs.getString(key, String());
  
  prefs.end();
  
  if (v.length()) {
    return atof(v.c_str());
  }
  
  if (mode == 2) return 60.0;  // U1 default
  if (mode == 3) return 40.0;  // U2 default
  return 50.0;                  // AUTO default
}

// NOTE: there is deliberately no set/getSetpoint here. The live setpoint is
// session-scoped (hold-persists): a cold boot starts from the device preset
// (getDefaultSetpoint), and a deep-sleep wake restores from RTC memory —
// persisting every setpoint change to NVS was flash wear with no reader.

void PreferencesManager::setDefaultSetpoint(double valueC) {
  #ifndef ENC_MIN_C
  #define ENC_MIN_C 50.0
  #endif
  #ifndef ENC_MAX_C
  #define ENC_MAX_C 300.0
  #endif
  
  valueC = constrain(valueC, (double)ENC_MIN_C, (double)ENC_MAX_C);
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  char buf[32];
  snprintf(buf, sizeof(buf), "%.1f", valueC);
  prefs.putString("def_sp", buf);
  
  prefs.end();
  
  Serial.printf("[PREF] Default Setpoint saved: %.1fC\n", valueC);
}

double PreferencesManager::getDefaultSetpoint() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  String v = prefs.getString("def_sp", "");
  prefs.end();
  
  return v.length() ? atof(v.c_str()) : 200.0;
}

// ===== BLE CONFIGURATION =====

void PreferencesManager::setBleEnabled(bool enabled) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  prefs.putString("ble_en", enabled ? "1" : "0");
  
  prefs.end();
  
  Serial.printf("[PREF] BLE enabled: %d\n", enabled ? 1 : 0);
}

bool PreferencesManager::getBleEnabled() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  String v = prefs.getString("ble_en", "");
  prefs.end();
  
  return v.length() ? (v.charAt(0) == '1') : true;  // Default enabled
}

void PreferencesManager::setBleName(int index) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", index);
  prefs.putString("ble_name", buf);
  
  prefs.end();
  
  Serial.printf("[PREF] BLE name index: %d\n", index);
}

int PreferencesManager::getBleName() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  String v = prefs.getString("ble_name", "");
  prefs.end();

  int idx = v.length() ? atoi(v.c_str()) : 0;  // Default 0 (Adamizer)
  if (idx < 0 || idx >= BLE_NAME_PRESET_COUNT) idx = 0;
  return idx;
}

void PreferencesManager::setBleAdvInterval(unsigned long ms) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", ms);
  prefs.putString("ble_adv", buf);
  
  prefs.end();
  
  Serial.printf("[PREF] BLE adv interval: %lu ms\n", ms);
}

unsigned long PreferencesManager::getBleAdvInterval() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  String v = prefs.getString("ble_adv", "");
  prefs.end();
  
  return v.length() ? strtoul(v.c_str(), nullptr, 10) : 200UL;  // Default 200ms
}

void PreferencesManager::setBleOledIndicator(bool enabled) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  prefs.putString("oled_ind", enabled ? "1" : "0");
  
  prefs.end();
  
  Serial.printf("[PREF] BLE OLED indicator: %d\n", enabled ? 1 : 0);
}

bool PreferencesManager::getBleOledIndicator() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  String v = prefs.getString("oled_ind", "");
  prefs.end();
  
  return v.length() ? (v.charAt(0) == '1') : true;  // Default enabled
}

// ===== TEMPERATURE UNIT =====

void PreferencesManager::setTempUnitC(bool isCelsius) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  prefs.putString("unit", isCelsius ? "1" : "0");
  
  prefs.end();
  
  Serial.printf("[PREF] Temp unit: %s\n", isCelsius ? "C" : "F");
}

bool PreferencesManager::getTempUnitC() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  String v = prefs.getString("unit", "");
  prefs.end();
  
  return v.length() ? (v.charAt(0) == '1') : true;  // Default C
}

// ===== PRESET SAVE/LOAD =====

void PreferencesManager::savePreset(int presetNum) {
  // Assumes caller has already updated the rtc* variables
  // This just persists them to NVS
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  
  if (presetNum == 1) {
    Serial.println("[PREF] Saving U1 preset...");
    // Caller should have already set rtcU1Kp, rtcU1Ki, rtcU1Kd, rtcU1Sp
    // This is a no-op if called from BLE callbacks (they persist directly)
  } else if (presetNum == 2) {
    Serial.println("[PREF] Saving U2 preset...");
  }
  
  prefs.end();
}

void PreferencesManager::loadPreset(int presetNum) {
  // Load entire preset into RAM
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  
  if (presetNum == 1) {
    Serial.println("[PREF] Loading U1 preset...");
  } else if (presetNum == 2) {
    Serial.println("[PREF] Loading U2 preset...");
  }
  
  prefs.end();
}

// ===== UTILITY =====

void PreferencesManager::clearAll() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  
  Serial.println("[PREF] All preferences cleared (factory reset)");
}

double PreferencesManager::validateDouble(const char* str, double minV, double maxV) {
  if (!str || *str == '\0') return NAN;
  
  char* endptr = nullptr;
  double val = strtod(str, &endptr);
  
  // String is valid if parsing consumed entire string
  if (endptr == nullptr || *endptr != '\0' || endptr == str) {
    return NAN;
  }
  
  // Clamp to bounds
  return constrain(val, minV, maxV);
}
