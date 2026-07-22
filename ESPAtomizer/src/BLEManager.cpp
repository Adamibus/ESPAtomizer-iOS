#include "BLEManager.h"
#include "PreferencesManager.h"
#include "MenuManager.h"
#include "ble.h"
#include "config.h"
#include "encoder.h"

#if USE_BLE

GlobalState* BLEManager::pGState = nullptr;
bool BLEManager::isConnectedFlag = false;

static bool isValidNumericString(const char* str) {
  if (str == nullptr || *str == '\0') return false;
  bool seenDot = false;
  bool seenDigit = false;
  const char* start = str;
  while (*str) {
    if (*str == '.') {
      if (seenDot) return false;
      seenDot = true;
    } else if (*str == '+' || *str == '-') {
      if (str != start) return false;
    } else if (*str >= '0' && *str <= '9') {
      seenDigit = true;
    } else {
      return false;
    }
    str++;
  }
  return seenDigit;
}

// Reports the outcome of a processed write back to the app over UUID_STATUS, so a rejected
// value is visible in the UI instead of only appearing in the serial log. `field` is a short
// code (e.g. "SP", "KP", "MODE"); `reason` is a short code too (e.g. "range", "fmt").
static void bleReportStatus(GlobalState* pGState, const char* field, bool ok, const char* reason = nullptr) {
  if (!pGState || !pGState->ble.chStatus) return;
  char buf[32];
  if (ok) {
    snprintf(buf, sizeof(buf), "OK:%s", field);
  } else {
    snprintf(buf, sizeof(buf), "ERR:%s:%s", field, reason ? reason : "invalid");
  }
  pGState->ble.chStatus->setValue(std::string(buf));
  pGState->ble.chStatus->notify();
}

// BLE callback class (moved from .ino)
class BLEManagerCallbacks : public NimBLECharacteristicCallbacks {
private:
  GlobalState* pGState = nullptr;
  
public:
  BLEManagerCallbacks(GlobalState& gState) : pGState(&gState) {}
  
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& connInfo) override {
    if (!pGState) return;
    
    std::string uuid = c->getUUID().toString();
    std::string val = c->getValue();
    
    // Safe numeric conversion
    auto toF = [&](const std::string &s) -> double {
      if (!isValidNumericString(s.c_str())) {
        Serial.printf("[BLE] ERROR: Invalid numeric format: '%s'\n", s.c_str());
        return NAN;
      }
      return atof(s.c_str());
    };
    
    // ===== ENABLE =====
    if (uuid == UUID_ENABLE) {
      if (!val.empty()) {
        bool en = (val[0] != '0');
        pGState->pidController.systemEnabled = en;
        if (!en) {
          pGState->pidController.pidOutput = 0;
        }
        // Echo the accepted value so readback is truthful, and mark it
        // published so the loop() watcher doesn't notify it back at the app.
        c->setValue(en ? "1" : "0");
        pGState->ble.pubEnable = en;
        pGState->ble.pubEnableSeeded = true;
        Serial.printf("[BLE] System enabled: %d\n", en ? 1 : 0);
        bleReportStatus(pGState, "EN", true);
      } else {
        bleReportStatus(pGState, "EN", false, "empty");
      }
    }

    // ===== SETPOINT =====
    else if (uuid == UUID_SETPOINT) {
      double newSp = toF(val);
      if (!isnan(newSp) && newSp >= (double)ENC_MIN_C && newSp <= (double)ENC_MAX_C) {
        // No NVS write here: the live setpoint is deliberately session-scoped
        // (hold-persists) — a cold boot starts from the device preset, so
        // persisting every slider move was pure flash wear with no reader.
        pGState->pidController.setpointC = constrain(newSp, (double)ENC_MIN_C, (double)ENC_MAX_C);
        // Echo accepted value + mark published (see ENABLE above).
        {
          char b[16];
          snprintf(b, sizeof(b), "%.1f", pGState->pidController.setpointC);
          c->setValue(std::string(b));
          pGState->ble.pubSetpoint = pGState->pidController.setpointC;
        }
        Serial.printf("[BLE] Setpoint set to %.1fC\n", pGState->pidController.setpointC);
        bleReportStatus(pGState, "SP", true);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid setpoint %.1f\n", newSp);
        bleReportStatus(pGState, "SP", false, isnan(newSp) ? "fmt" : "range");
      }
    }

    // ===== KP =====
    else if (uuid == UUID_KP) {
      double newKp = toF(val);
      if (!isnan(newKp) && newKp >= 0.1 && newKp <= 100.0) {
        pGState->pidController.Kp = constrain(newKp, 0.1, 100.0);
        PreferencesManager::setKp(pGState->pidController.Kp, pGState->pidController.pidMode);
        { char b[16]; snprintf(b, sizeof(b), "%.3f", pGState->pidController.Kp); c->setValue(std::string(b)); }
        Serial.printf("[BLE] Kp set to %.3f\n", pGState->pidController.Kp);
        bleReportStatus(pGState, "KP", true);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid Kp %.3f\n", newKp);
        bleReportStatus(pGState, "KP", false, isnan(newKp) ? "fmt" : "range");
      }
    }

    // ===== KI =====
    else if (uuid == UUID_KI) {
      double newKi = toF(val);
      if (!isnan(newKi) && newKi >= 0.01 && newKi <= 10.0) {
        pGState->pidController.Ki = constrain(newKi, 0.01, 10.0);
        PreferencesManager::setKi(pGState->pidController.Ki, pGState->pidController.pidMode);
        { char b[16]; snprintf(b, sizeof(b), "%.3f", pGState->pidController.Ki); c->setValue(std::string(b)); }
        Serial.printf("[BLE] Ki set to %.3f\n", pGState->pidController.Ki);
        bleReportStatus(pGState, "KI", true);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid Ki %.3f\n", newKi);
        bleReportStatus(pGState, "KI", false, isnan(newKi) ? "fmt" : "range");
      }
    }

    // ===== KD =====
    else if (uuid == UUID_KD) {
      double newKd = toF(val);
      if (!isnan(newKd) && newKd >= 0.1 && newKd <= 1000.0) {
        pGState->pidController.Kd = constrain(newKd, 0.1, 1000.0);
        PreferencesManager::setKd(pGState->pidController.Kd, pGState->pidController.pidMode);
        { char b[16]; snprintf(b, sizeof(b), "%.3f", pGState->pidController.Kd); c->setValue(std::string(b)); }
        Serial.printf("[BLE] Kd set to %.3f\n", pGState->pidController.Kd);
        bleReportStatus(pGState, "KD", true);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid Kd %.3f\n", newKd);
        bleReportStatus(pGState, "KD", false, isnan(newKd) ? "fmt" : "range");
      }
    }

    // ===== DEFAULT SETPOINT =====
    else if (uuid == UUID_DEFAULT_SP) {
      double v = toF(val);
      if (!val.empty() && !isnan(v)) {
        pGState->pidController.defaultSetpoint = v;
        PreferencesManager::setDefaultSetpoint(v);
        { char b[16]; snprintf(b, sizeof(b), "%.1f", v); c->setValue(std::string(b)); }
        Serial.printf("[BLE] Default SP set to %.1fC\n", v);
        bleReportStatus(pGState, "DSP", true);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid default setpoint '%s'\n", val.c_str());
        bleReportStatus(pGState, "DSP", false, "fmt");
      }
    }

    // ===== UNIT =====
    else if (uuid == UUID_UNIT) {
      if (!val.empty()) {
        char c = val[0];
        bool isC = (c == 'C' || c == 'c' || c == '0');
        pGState->menu.tempUnitIsC = isC;
        PreferencesManager::setTempUnitC(isC);
        pGState->ble.chUnit->setValue(isC ? "C" : "F");   // normalize readback ("c"/"0" → "C")
        Serial.printf("[BLE] Unit set to %s\n", isC ? "C" : "F");
        bleReportStatus(pGState, "UNIT", true);
      } else {
        bleReportStatus(pGState, "UNIT", false, "empty");
      }
    }

    // ===== OUTPUT (manual mode) =====
    else if (uuid == UUID_OUT) {
      int newOut = constrain(atoi(val.c_str()), 0, (int)PWM_MAX);
      pGState->pidController.pidOutput = newOut;
      // Echo accepted value + mark published (see ENABLE above).
      { char b[12]; snprintf(b, sizeof(b), "%d", newOut); c->setValue(std::string(b)); }
      pGState->ble.pubOut = newOut;
      Serial.printf("[BLE] Output set to %d\n", newOut);
      bleReportStatus(pGState, "OUT", true);
    }

    // ===== MODE =====
    // Fix F2: the app writes the PID mode here (Auto=0, Manual=1, U1=2, U2=3, Config=4).
    // Previously there was no handler, so mode changes from the phone were silently ignored.
    else if (uuid == UUID_MODE) {
      if (!val.empty()) {
        int m = atoi(val.c_str());
        if (m >= 0 && m <= 4) {
          applyPidMode(m);   // applies mode and echoes back over chMode/chModeRead
          Serial.printf("[BLE] Mode set to %d\n", m);
          bleReportStatus(pGState, "MODE", true);
        } else {
          Serial.printf("[BLE] WARNING: Rejected invalid mode %d\n", m);
          bleReportStatus(pGState, "MODE", false, "range");
        }
      } else {
        bleReportStatus(pGState, "MODE", false, "empty");
      }
    }

    pGState->diagnostics.bleWrites++;
  }
};

void BLEManager::init(GlobalState& gState) {
  pGState = &gState;
  
  Serial.println("[BLEManager] Initializing BLE...");
  
  NimBLEDevice::init(BLE_NAME_PRESETS[PreferencesManager::getBleName()]);
  
  pGState->ble.server = NimBLEDevice::createServer();
  NimBLEService* svc = pGState->ble.server->createService(BLE_SVC_UUID);
  
  auto propsRWEnc = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
                    NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC;
  auto propsRNEnc = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY |
                    NIMBLE_PROPERTY::READ_ENC;
  
  // Create characteristics.
  // Enable/Setpoint/Out carry NOTIFY too: power, target and duty all change
  // on the DEVICE side (dial, safety shutoff, PID loop), and without notify
  // the app had no way to hear about it — its Power/Target/Output UI went
  // stale until reconnect. The loop() watcher in ESPAtomizer.ino publishes them.
  pGState->ble.chEnable = svc->createCharacteristic(UUID_ENABLE, propsRWEnc | NIMBLE_PROPERTY::NOTIFY);
  pGState->ble.chSetpoint = svc->createCharacteristic(UUID_SETPOINT, propsRWEnc | NIMBLE_PROPERTY::NOTIFY);
  pGState->ble.chKp = svc->createCharacteristic(UUID_KP, propsRWEnc);
  pGState->ble.chKi = svc->createCharacteristic(UUID_KI, propsRWEnc);
  pGState->ble.chKd = svc->createCharacteristic(UUID_KD, propsRWEnc);
  pGState->ble.chMode = svc->createCharacteristic(UUID_MODE, propsRWEnc);
  pGState->ble.chTemp = svc->createCharacteristic(UUID_TEMP, propsRNEnc);
  pGState->ble.chOut = svc->createCharacteristic(UUID_OUT, propsRWEnc | NIMBLE_PROPERTY::NOTIFY);
  pGState->ble.chBat = svc->createCharacteristic(UUID_BAT, propsRNEnc);
  pGState->ble.chModeRead = svc->createCharacteristic(UUID_MODE_READ, propsRNEnc);
  pGState->ble.chDefaultSp = svc->createCharacteristic(UUID_DEFAULT_SP, propsRWEnc);
  pGState->ble.chUnit = svc->createCharacteristic(UUID_UNIT, propsRWEnc | NIMBLE_PROPERTY::NOTIFY);
  pGState->ble.chTcStatus = svc->createCharacteristic(UUID_TC_STATUS, propsRNEnc);

  // Write-result ack characteristic: notifies "OK:<FIELD>" / "ERR:<FIELD>:<reason>" after
  // each processed write, so a rejected value is visible in the app UI (previously only
  // logged to serial). See bleReportStatus() above.
  pGState->ble.chStatus = svc->createCharacteristic(UUID_STATUS, propsRNEnc);
  pGState->ble.chStatus->setValue("OK:INIT");

  // Protocol/contract version (read-only). The app reads this and warns if it doesn't match the
  // version it was built against. Local pointer is enough — never written or notified after init.
  {
    NimBLECharacteristic* chVer = svc->createCharacteristic(UUID_PROTOCOL_VERSION,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC);
    char vbuf[8];
    snprintf(vbuf, sizeof(vbuf), "%d", BLE_PROTOCOL_VERSION);
    chVer->setValue(std::string(vbuf));
  }

  // Initialize TC status
  pGState->ble.chTcStatus->setValue("1");

  // Set callbacks
  static BLEManagerCallbacks cb(gState);
  pGState->ble.chEnable->setCallbacks(&cb);
  pGState->ble.chSetpoint->setCallbacks(&cb);
  pGState->ble.chKp->setCallbacks(&cb);
  pGState->ble.chKi->setCallbacks(&cb);
  pGState->ble.chKd->setCallbacks(&cb);
  pGState->ble.chMode->setCallbacks(&cb);
  pGState->ble.chOut->setCallbacks(&cb);
  pGState->ble.chUnit->setCallbacks(&cb);

  svc->start();
  
  // Start advertising
  // Fix F4: use "Just Works" pairing (bonding + secure connections, but no MITM).
  // The previous MITM + DISPLAY_YESNO combo required a numeric-comparison confirm that this
  // device has no UI to complete, so iOS pairing would stall. NO_INPUT_OUTPUT lets iOS pair
  // silently. (To require MITM later, restore DISPLAY_YESNO AND implement an onConfirmPIN
  // callback that shows the code on the OLED and confirms with the encoder.)
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData advData;
  advData.setName(BLE_NAME_PRESETS[PreferencesManager::getBleName()]);
  adv->setAdvertisementData(advData);
  adv->addServiceUUID(BLE_SVC_UUID);
  adv->setConnectableMode(1);
  adv->start();
  
  // Server callbacks for connect/disconnect
  class ServerCallbacks : public NimBLEServerCallbacks {
  private:
    GlobalState* pGState = nullptr;
  public:
    ServerCallbacks(GlobalState& gState) : pGState(&gState) {}
    
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
      isConnectedFlag = true;
      pGState->ble.animExpireMs = millis() + 1000UL;
      pGState->ble.animIsConnect = true;
      snprintf(pGState->ble.lastEvent, sizeof(pGState->ble.lastEvent), "conn@%lus", millis() / 1000UL);
      Serial.println(F("[BLE] Client connected"));
    }
    
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
      isConnectedFlag = false;
      pGState->ble.animExpireMs = millis() + 1000UL;
      pGState->ble.animIsConnect = false;
      snprintf(pGState->ble.lastEvent, sizeof(pGState->ble.lastEvent), "disc@%lus", millis() / 1000UL);
      NimBLEDevice::getAdvertising()->start();
      Serial.println(F("[BLE] Client disconnected; advertising restarted"));
    }
  };
  
  static ServerCallbacks serverCb(gState);
  pGState->ble.server->setCallbacks(&serverCb);
  
  Serial.println("[BLEManager] BLE initialized");
}

bool BLEManager::isConnected() {
  return isConnectedFlag;
}

const char* BLEManager::getLastEvent() {
  if (!pGState) return "no-mgr";
  return pGState->ble.lastEvent;
}

void BLEManager::seedCharacteristicValues() {
  if (!pGState || !pGState->ble.server) return;

  char b[16];
  auto& pid = pGState->pidController;

  if (pGState->ble.chEnable) {
    pGState->ble.chEnable->setValue(pid.systemEnabled ? "1" : "0");
    pGState->ble.pubEnable = pid.systemEnabled;
    pGState->ble.pubEnableSeeded = true;
  }
  if (pGState->ble.chSetpoint) {
    snprintf(b, sizeof(b), "%.1f", pid.setpointC);
    pGState->ble.chSetpoint->setValue(std::string(b));
    pGState->ble.pubSetpoint = pid.setpointC;
  }
  if (pGState->ble.chKp) { snprintf(b, sizeof(b), "%.3f", pid.Kp); pGState->ble.chKp->setValue(std::string(b)); }
  if (pGState->ble.chKi) { snprintf(b, sizeof(b), "%.3f", pid.Ki); pGState->ble.chKi->setValue(std::string(b)); }
  if (pGState->ble.chKd) { snprintf(b, sizeof(b), "%.3f", pid.Kd); pGState->ble.chKd->setValue(std::string(b)); }
  if (pGState->ble.chOut) {
    snprintf(b, sizeof(b), "%d", (int)pid.pidOutput);
    pGState->ble.chOut->setValue(std::string(b));
    pGState->ble.pubOut = (int)pid.pidOutput;
  }
  if (pGState->ble.chDefaultSp) { snprintf(b, sizeof(b), "%.1f", pid.defaultSetpoint); pGState->ble.chDefaultSp->setValue(std::string(b)); }
  if (pGState->ble.chUnit) { pGState->ble.chUnit->setValue(pGState->menu.tempUnitIsC ? "C" : "F"); }
  // Mode chars: on an RTC wake applyPidMode() already re-seeded them, but on a
  // cold boot nothing did — seed both so the app's first read is never empty.
  if (pGState->ble.chModeRead) { snprintf(b, sizeof(b), "%d", pid.pidMode); pGState->ble.chModeRead->setValue(std::string(b)); }
  if (pGState->ble.chMode) { pGState->ble.chMode->setValue(MenuManager::getMenuItemName(pid.pidMode)); }

  Serial.println("[BLE] Characteristic values seeded from state");
}

void BLEManager::setDeviceName(int nameIndex) {
  if (!pGState || !pGState->ble.server) return;
  
  if (nameIndex < 0 || nameIndex >= BLE_NAME_PRESET_COUNT) nameIndex = 0;  // Safety clamp
  
  const char* newName = BLE_NAME_PRESETS[nameIndex];
  NimBLEDevice::init(newName);
  
  // Restart advertising with new name
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->stop();
  
  NimBLEAdvertisementData advData;
  advData.setName(newName);
  adv->setAdvertisementData(advData);
  adv->start();
  
  PreferencesManager::setBleName(nameIndex);
  Serial.printf("[BLE] Device name changed to '%s'\n", newName);
}

#endif  // USE_BLE
