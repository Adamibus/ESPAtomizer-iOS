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
        pGState->pidController.systemEnabled = (val[0] != '0');
        if (!pGState->pidController.systemEnabled) {
          pGState->pidController.pidOutput = 0;
        }
        Serial.printf("[BLE] System enabled: %d\n", pGState->pidController.systemEnabled ? 1 : 0);
      }
    }
    
    // ===== SETPOINT =====
    else if (uuid == UUID_SETPOINT) {
      double newSp = toF(val);
      if (!isnan(newSp) && newSp >= (double)ENC_MIN_C && newSp <= (double)ENC_MAX_C) {
        pGState->pidController.setpointC = constrain(newSp, (double)ENC_MIN_C, (double)ENC_MAX_C);
        PreferencesManager::setSetpoint(pGState->pidController.setpointC, pGState->pidController.pidMode);
        Serial.printf("[BLE] Setpoint set to %.1fC\n", pGState->pidController.setpointC);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid setpoint %.1f\n", newSp);
      }
    }
    
    // ===== KP =====
    else if (uuid == UUID_KP) {
      double newKp = toF(val);
      if (!isnan(newKp) && newKp >= 0.1 && newKp <= 100.0) {
        pGState->pidController.Kp = constrain(newKp, 0.1, 100.0);
        PreferencesManager::setKp(pGState->pidController.Kp, pGState->pidController.pidMode);
        Serial.printf("[BLE] Kp set to %.3f\n", pGState->pidController.Kp);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid Kp %.3f\n", newKp);
      }
    }
    
    // ===== KI =====
    else if (uuid == UUID_KI) {
      double newKi = toF(val);
      if (!isnan(newKi) && newKi >= 0.01 && newKi <= 10.0) {
        pGState->pidController.Ki = constrain(newKi, 0.01, 10.0);
        PreferencesManager::setKi(pGState->pidController.Ki, pGState->pidController.pidMode);
        Serial.printf("[BLE] Ki set to %.3f\n", pGState->pidController.Ki);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid Ki %.3f\n", newKi);
      }
    }
    
    // ===== KD =====
    else if (uuid == UUID_KD) {
      double newKd = toF(val);
      if (!isnan(newKd) && newKd >= 0.1 && newKd <= 1000.0) {
        pGState->pidController.Kd = constrain(newKd, 0.1, 1000.0);
        PreferencesManager::setKd(pGState->pidController.Kd, pGState->pidController.pidMode);
        Serial.printf("[BLE] Kd set to %.3f\n", pGState->pidController.Kd);
      } else {
        Serial.printf("[BLE] WARNING: Rejected invalid Kd %.3f\n", newKd);
      }
    }
    
    // ===== DEFAULT SETPOINT =====
    else if (uuid == UUID_DEFAULT_SP) {
      if (!val.empty()) {
        double v = toF(val);
        pGState->pidController.defaultSetpoint = v;
        PreferencesManager::setDefaultSetpoint(v);
        Serial.printf("[BLE] Default SP set to %.1fC\n", v);
      }
    }
    
    // ===== UNIT =====
    else if (uuid == UUID_UNIT) {
      if (!val.empty()) {
        char c = val[0];
        bool isC = (c == 'C' || c == 'c' || c == '0');
        pGState->menu.tempUnitIsC = isC;
        PreferencesManager::setTempUnitC(isC);
        Serial.printf("[BLE] Unit set to %s\n", isC ? "C" : "F");
      }
    }
    
    // ===== OUTPUT (manual mode) =====
    else if (uuid == UUID_OUT) {
      int newOut = constrain(atoi(val.c_str()), 0, (int)PWM_MAX);
      pGState->pidController.pidOutput = newOut;
      Serial.printf("[BLE] Output set to %d\n", newOut);
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
  
  // Create characteristics
  pGState->ble.chEnable = svc->createCharacteristic(UUID_ENABLE, propsRWEnc);
  pGState->ble.chSetpoint = svc->createCharacteristic(UUID_SETPOINT, propsRWEnc);
  pGState->ble.chKp = svc->createCharacteristic(UUID_KP, propsRWEnc);
  pGState->ble.chKi = svc->createCharacteristic(UUID_KI, propsRWEnc);
  pGState->ble.chKd = svc->createCharacteristic(UUID_KD, propsRWEnc);
  pGState->ble.chMode = svc->createCharacteristic(UUID_MODE, propsRWEnc);
  pGState->ble.chTemp = svc->createCharacteristic(UUID_TEMP, propsRNEnc);
  pGState->ble.chOut = svc->createCharacteristic(UUID_OUT, propsRWEnc);
  pGState->ble.chBat = svc->createCharacteristic(UUID_BAT, propsRNEnc);
  pGState->ble.chModeRead = svc->createCharacteristic(UUID_MODE_READ, propsRNEnc);
  pGState->ble.chDefaultSp = svc->createCharacteristic(UUID_DEFAULT_SP, propsRWEnc);
  pGState->ble.chUnit = svc->createCharacteristic(UUID_UNIT, propsRWEnc | NIMBLE_PROPERTY::NOTIFY);
  pGState->ble.chTcStatus = svc->createCharacteristic(UUID_TC_STATUS, propsRNEnc);
  
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
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO);
  
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

void BLEManager::update() {
  // BLE stack handles itself; no explicit update needed
}

bool BLEManager::isConnected() {
  return isConnectedFlag;
}

const char* BLEManager::getLastEvent() {
  if (!pGState) return "no-mgr";
  return pGState->ble.lastEvent;
}

void BLEManager::notifyAllCharacteristics() {
  if (!pGState || !pGState->ble.server) return;
  
  // Notify all readable characteristics to force client update
  if (pGState->ble.chTemp) pGState->ble.chTemp->notify();
  if (pGState->ble.chBat) pGState->ble.chBat->notify();
  if (pGState->ble.chModeRead) pGState->ble.chModeRead->notify();
  if (pGState->ble.chUnit) pGState->ble.chUnit->notify();
  if (pGState->ble.chTcStatus) pGState->ble.chTcStatus->notify();
  
  Serial.println("[BLE] All characteristics notified");
}

void BLEManager::clearBonds() {
  #if defined(HAVE_NIMBLE_STORE_HDR)
  extern int ble_store_clear(void);
  int ret = ble_store_clear();
  Serial.printf("[BLE] Bonds cleared (ret=%d)\n", ret);
  #else
  Serial.println("[BLE] Bond clearing not available on this platform");
  #endif
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
