#pragma once
#include "config.h"

// BLE name presets are used by menus/UI even when BLE support is compiled out.
static const char* BLE_NAME_PRESETS[] = { "Adamizer", "ESPAtom", "Atomizer" };
static const int BLE_NAME_PRESET_COUNT = (sizeof(BLE_NAME_PRESETS) / sizeof(BLE_NAME_PRESETS[0]));

#if USE_BLE

#include <Arduino.h>
#include "StateManager.h"
#include <NimBLEDevice.h>
#include <NimBLEAdvertising.h>
#include <NimBLEServer.h>
#include <NimBLECharacteristic.h>
#include <NimBLEUtils.h>
#include <string>

extern GlobalState gState;

// BLE UUIDs (randomly generated)
static const char* BLE_SVC_UUID = "b09aa6b5-0f22-4d9c-9dbc-6e3c7d9b2f0a";
static const char* UUID_ENABLE  = "3f1a0001-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_SETPOINT= "3f1a0002-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_KP      = "3f1a0003-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_KI      = "3f1a0004-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_KD      = "3f1a0005-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_MODE    = "3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_TEMP    = "3f1a0007-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_OUT     = "3f1a0008-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_BAT     = "3f1a0009-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_MODE_READ = "3f1a0006-2a8d-4a54-8f2f-b7cd2b4b8002";
static const char* UUID_DEFAULT_SP = "3f1a000a-2a8d-4a54-8f2f-b7cd2b4b8001";
static const char* UUID_UNIT = "3f1a000b-2a8d-4a54-8f2f-b7cd2b4b8001"; // "C" or "F"
static const char* UUID_TC_STATUS     = "3f1a000c-2a8d-4a54-8f2f-b7cd2b4b8001"; // Thermocouple connection status
static const char* UUID_SAVE_SCRIPT   = "3f1a00ff-0000-0000-0000-000000000001"; // Per-profile script/note storage; app writes "SAVE:<profile>:<payload>"
static const char* UUID_STATUS        = "3f1a000d-2a8d-4a54-8f2f-b7cd2b4b8001"; // Write-result ack: notifies "OK:<FIELD>" or "ERR:<FIELD>:<reason>" after each processed write
static const char* UUID_PROTOCOL_VERSION = "3f1a000e-2a8d-4a54-8f2f-b7cd2b4b8001"; // Read-only integer: BLE contract version (see BLE_PROTOCOL_VERSION)

// BLE contract version. Bump when a characteristic UUID or payload format changes in a way the
// app must know about. MUST stay in sync with docs/BLE-PROTOCOL.md and the app's
// expectedProtocolVersion (AtomizerViewModel.swift). The contract test tools/ble_contract_check.py
// verifies firmware and app agree.
#define BLE_PROTOCOL_VERSION 1

// Forward declare function used by callbacks in sketch
void applyPidMode(int mode);

// BLE implementation is now handled by BLEManager.

// Convenience helper for connection status
static inline bool bleIsConnected() {
	return (gState.ble.server != nullptr && gState.ble.server->getConnectedCount() > 0);
}

#endif // USE_BLE
