#include "SafetyManager.h"
#include "config.h"

GlobalState* SafetyManager::pGState = nullptr;

static bool batteryLowBypass() {
#if ALLOW_BOOT_WITHOUT_BATTERY
  return true;
#else
  return false;
#endif
}

void SafetyManager::init(GlobalState& gState) {
  pGState = &gState;
  Serial.println("[SafetyManager] Initialized");
}

void SafetyManager::update() {
  if (!pGState) return;
  
  unsigned long now = millis();
  SafetyState& safety = pGState->safety;

#if ENABLE_WATCHDOG
  // ===== WATCHDOG CHECK =====
  // Detect main loop stall
  if (now - safety.watchdogLastLoopMs > WATCHDOG_TIMEOUT_MS) {
    safety.watchdogFaulted = true;
    Serial.printf("[SAFETY] WATCHDOG TIMEOUT: Loop stalled for %lu ms\n", 
                  now - safety.watchdogLastLoopMs);
  }
  
  // Detect PID compute timeout.
  // Only track timeout when auto PID operation is expected.
  if (pGState->pidController.systemEnabled && !pGState->pidController.manualMode) {
    if (safety.pidLastComputeMs == 0) {
      // Initialize baseline so we don't immediately trigger a timeout
      safety.pidLastComputeMs = now;
    } else if (now - safety.pidLastComputeMs > PID_TIMEOUT_MS) {
      safety.watchdogFaulted = true;
      Serial.printf("[SAFETY] PID TIMEOUT: Not computed in %lu ms\n", 
                    now - safety.pidLastComputeMs);
    }
  }
#endif

#if ENABLE_THERMAL_SAFETY
  // ===== THERMAL RUNAWAY CHECK =====
  // 1. Absolute max temperature
  if (pGState->pidController.inputC > THERMAL_RUNAWAY_TEMP_C) {
    safety.thermalRunawayFaulted = true;
    safety.thermalRunawayMs = now;
    Serial.printf("[SAFETY] THERMAL RUNAWAY: Temp %.1fC exceeds limit %.1fC\n",
                  pGState->pidController.inputC, (double)THERMAL_RUNAWAY_TEMP_C);
  }
  
  // 2. Setpoint margin (temp > setpoint + margin)
  if (pGState->pidController.inputC > (pGState->pidController.setpointC + THERMAL_RUNAWAY_MARGIN_C)) {
    safety.thermalRunawayFaulted = true;
    safety.thermalRunawayMs = now;
    Serial.printf("[SAFETY] THERMAL MARGIN: Temp %.1fC > Setpoint %.1fC + margin %.1fC\n",
                  pGState->pidController.inputC, pGState->pidController.setpointC, 
                  (double)THERMAL_RUNAWAY_MARGIN_C);
  }
  
  // 3. Max on-time at high power without temp rise
  if (pGState->pidController.pidOutput > MAX_PWM_THRESHOLD) {
    if (safety.maxOnTimeStartMs == 0) {
      safety.maxOnTimeStartMs = now;
    } else if (now - safety.maxOnTimeStartMs > MAX_ON_TIME_MS) {
      safety.thermalRunawayFaulted = true;
      safety.thermalRunawayMs = now;
      Serial.printf("[SAFETY] MAX ON TIME: High power for %lu ms without temp rise\n",
                    now - safety.maxOnTimeStartMs);
    }
  } else {
    safety.maxOnTimeStartMs = 0;  // Reset timer if output drops
  }
#endif

  // ===== BATTERY SAFETY CHECK =====
  if (pGState->battery.voltage < BAT_CUTOFF_V) {
    if (batteryLowBypass()) {
      Serial.printf("[SAFETY] BATTERY LOW: %.2fV below cutoff %.2fV, but USB bypass is enabled\n",
                    pGState->battery.voltage, (double)BAT_CUTOFF_V);
    } else {
      Serial.printf("[SAFETY] BATTERY LOW: %.2fV below cutoff %.2fV\n",
                    pGState->battery.voltage, (double)BAT_CUTOFF_V);
    }
  }
  
  // ===== SENSOR FAULT CHECK =====
  // (markSensorRead() updates sensor fault state)
  if (pGState->sensorFault.faulted) {
    Serial.printf("[SAFETY] SENSOR FAULT: %d consecutive bad reads\n",
                  pGState->sensorFault.faultCount);
  }
}

bool SafetyManager::isSafe() {
  if (!pGState) return false;
  
  SafetyState& safety = pGState->safety;
  SensorFaultState& sensorFault = pGState->sensorFault;
  BatteryState& battery = pGState->battery;
  
  // Not safe if any critical fault
  bool safe = !safety.watchdogFaulted && 
              !safety.thermalRunawayFaulted && 
              !sensorFault.faulted &&
              (battery.voltage >= BAT_CUTOFF_V || batteryLowBypass());
  
  return safe;
}

const char* SafetyManager::getLastFault() {
  if (!pGState) return "No manager";
  
  SafetyState& safety = pGState->safety;
  
  if (safety.watchdogFaulted) return "Watchdog timeout";
  if (safety.thermalRunawayFaulted) return "Thermal runaway";
  
  SensorFaultState& sensorFault = pGState->sensorFault;
  if (sensorFault.faulted) return "Sensor fault";
  
  BatteryState& battery = pGState->battery;
  if (battery.voltage < BAT_CUTOFF_V && !batteryLowBypass()) return "Battery low";
  
  return "None";
}

void SafetyManager::clearFaults() {
  if (!pGState) return;
  
  SafetyState& safety = pGState->safety;
  SensorFaultState& sensorFault = pGState->sensorFault;
  
  safety.watchdogFaulted = false;
  safety.thermalRunawayFaulted = false;
  safety.maxOnTimeStartMs = 0;
  
  sensorFault.faulted = false;
  sensorFault.faultCount = 0;
  sensorFault.validCount = 0;
  
  Serial.println("[SafetyManager] Faults cleared");
}

void SafetyManager::markLoopEntry() {
  if (!pGState) return;
  pGState->safety.watchdogLastLoopMs = millis();
}

void SafetyManager::markPIDCompute() {
  if (!pGState) return;
  pGState->safety.pidLastComputeMs = millis();
}

void SafetyManager::markSensorRead(double tempC, bool valid) {
  if (!pGState) return;
  
  SensorFaultState& sensorFault = pGState->sensorFault;
  BatteryState& bat = pGState->battery;

  // If battery is below minimum or in cutoff, skip updating sensor fault
  // counters because low/invalid ADC on some platforms can produce spurious
  // sensor read failures during low-power conditions.
  if (!isnan(bat.voltage) && bat.voltage < BAT_MIN_V) {
    Serial.printf("[SAFETY] Skipping sensor fault accounting due to low battery: %.3fV\n", bat.voltage);
    // Do not modify valid/fault counts; keep previous state until battery recovers
    return;
  }
  
  if (valid && !isnan(tempC) && tempC >= ABSOLUTE_MIN_TEMP_C && tempC <= ABSOLUTE_MAX_TEMP_C) {
    // Valid reading
    sensorFault.validCount++;
    sensorFault.faultCount = 0;
    
    // Recover from fault after SENSOR_VALID_RECOVERY consecutive valid reads
    if (sensorFault.faulted && sensorFault.validCount >= SENSOR_VALID_RECOVERY) {
      sensorFault.faulted = false;
      Serial.printf("[SafetyManager] Sensor recovered after %d valid reads\n", SENSOR_VALID_RECOVERY);
    }
  } else {
    // Invalid reading
    sensorFault.faultCount++;
    sensorFault.validCount = 0;
    
    // Fault if SENSOR_FAULT_THRESHOLD consecutive bad reads
    if (!sensorFault.faulted && sensorFault.faultCount >= SENSOR_FAULT_THRESHOLD) {
      sensorFault.faulted = true;
      sensorFault.faultMs = millis();
      Serial.printf("[SafetyManager] SENSOR FAULT: %d consecutive bad reads\n", SENSOR_FAULT_THRESHOLD);
    }
  }
}

bool SafetyManager::getSensorFaulted() {
  if (!pGState) return false;
  return pGState->sensorFault.faulted;
}

bool SafetyManager::getThermalRunaway() {
  if (!pGState) return false;
  return pGState->safety.thermalRunawayFaulted;
}

bool SafetyManager::getWatchdogFaulted() {
  if (!pGState) return false;
  return pGState->safety.watchdogFaulted;
}
