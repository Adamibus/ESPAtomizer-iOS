# ESP-IDF example: set GATT attribute permissions (ble_gatts_set_attr_perm)

This example demonstrates how to create a simple NimBLE GATT service and then set
per-attribute permissions deterministically using the host API `ble_gatts_set_attr_perm()`
in an ESP‑IDF application. Use this as a migration reference when the Arduino core
doesn't expose the host-level helpers you need.

Notes:
- This example targets ESP‑IDF (v4.x / v5.x with NimBLE enabled). It is not an Arduino
  sketch. Build with `idf.py build` after configuring your ESP‑IDF environment.
- The exact include paths for NimBLE host headers can vary across ESP‑IDF/NimBLE versions.
  The example shows the canonical includes and usage; adjust include paths if your
  environment differs.

Build & flash (example):

1. Install and set up ESP‑IDF following Espressif's docs.
2. cd into this example folder and run:

```powershell
set IDF_PATH=C:\path\to\esp-idf
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash monitor
```

Replace `COM3` with your serial port.

After the example starts, it will create a GATT service and then call
`ble_gatts_set_attr_perm(handle, perm)` for the characteristic attribute. Use
an Android/iOS BLE scanner to connect and test that the attribute requires
encryption if the permission is set accordingly.
