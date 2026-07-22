#!/usr/bin/env python3
"""
BLE contract check — app <-> firmware.

Verifies that the iOS app and the ESP32 firmware agree on the BLE GATT contract, so the two
sides can't drift silently (the root cause of the mode/battery bugs and the "AUTO"/atoi mode
mismatch found earlier). Runs with no hardware and no dependencies — suitable for CI or a
pre-commit hook.

It checks:
  1. BLE contract version matches: firmware BLE_PROTOCOL_VERSION == app expectedProtocolVersion.
  2. Every characteristic UUID the app references is defined by the firmware (app ⊆ firmware).
  3. (Warning only) firmware characteristic UUIDs the app never references.

Exit code 0 = contract OK, 1 = mismatch (or sources not found).

Source of truth for humans: docs/BLE-PROTOCOL.md. This script enforces code-to-code agreement.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
FIRMWARE_BLE_H = REPO_ROOT / "ESPAtomizer" / "ble.h"
APP_VIEWMODEL = REPO_ROOT / "ESPAtomizer-iOS" / "ESPAtomizer" / "AtomizerViewModel.swift"

# 128-bit UUID like 3f1a0001-2a8d-4a54-8f2f-b7cd2b4b8001
UUID_RE = r"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"


def fail(msg):
    print(f"  ✗ {msg}")


def parse_firmware(text):
    """Return (uuid_value -> const_name dict, protocol_version int or None)."""
    uuids = {}
    # static const char* UUID_FOO = "....";   (also BLE_SVC_UUID)
    for m in re.finditer(r'(\b[A-Z0-9_]*UUID[A-Z0-9_]*)\s*=\s*"(' + UUID_RE + r')"', text):
        uuids[m.group(2).lower()] = m.group(1)
    ver = None
    vm = re.search(r'#define\s+BLE_PROTOCOL_VERSION\s+(\d+)', text)
    if vm:
        ver = int(vm.group(1))
    return uuids, ver


def parse_app(text):
    """Return (uuid_value -> swift_name dict, expected_version int or None)."""
    uuids = {}
    # private let uuidFoo = CBUUID(string: "....")   /  let serviceUUID = CBUUID(string: "....")
    for m in re.finditer(r'(\b\w+)\s*=\s*CBUUID\(string:\s*"(' + UUID_RE + r')"\)', text):
        uuids[m.group(2).lower()] = m.group(1)
    ver = None
    vm = re.search(r'expectedProtocolVersion\s*=\s*(\d+)', text)
    if vm:
        ver = int(vm.group(1))
    return uuids, ver


def main():
    ok = True

    for p in (FIRMWARE_BLE_H, APP_VIEWMODEL):
        if not p.exists():
            fail(f"source not found: {p}")
            return 1

    fw_uuids, fw_ver = parse_firmware(FIRMWARE_BLE_H.read_text())
    app_uuids, app_ver = parse_app(APP_VIEWMODEL.read_text())

    print("BLE contract check")
    print(f"  firmware: {FIRMWARE_BLE_H.relative_to(REPO_ROOT)}  ({len(fw_uuids)} UUIDs, version {fw_ver})")
    print(f"  app:      {APP_VIEWMODEL.relative_to(REPO_ROOT)}  ({len(app_uuids)} UUIDs, version {app_ver})")
    print()

    # 1. Protocol version agreement
    if fw_ver is None:
        fail("firmware BLE_PROTOCOL_VERSION not found in ble.h")
        ok = False
    if app_ver is None:
        fail("app expectedProtocolVersion not found in AtomizerViewModel.swift")
        ok = False
    if fw_ver is not None and app_ver is not None and fw_ver != app_ver:
        fail(f"protocol version mismatch: firmware={fw_ver}, app={app_ver}")
        ok = False

    # 2. Every UUID the app uses must exist in firmware
    for value, swift_name in sorted(app_uuids.items()):
        if value not in fw_uuids:
            fail(f"app '{swift_name}' uses UUID {value} that the firmware does not define")
            ok = False

    # 3. Firmware UUIDs the app never references (warning only — e.g. save-script is unused today)
    unreferenced = [f"{name} ({v})" for v, name in sorted(fw_uuids.items()) if v not in app_uuids]
    if unreferenced:
        print("  ⚠ firmware characteristics not referenced by the app (informational):")
        for u in unreferenced:
            print(f"      - {u}")
        print()

    if ok:
        print("  ✓ contract OK — app and firmware agree on UUIDs and protocol version")
        return 0
    print()
    print("  BLE contract check FAILED — app and firmware have drifted. See docs/BLE-PROTOCOL.md.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
