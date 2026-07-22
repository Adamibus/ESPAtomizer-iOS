**ESP‑IDF Migration Notes**

When the Arduino core does not expose host-level NimBLE APIs (for example
`ble_gatts_set_attr_perm`), you can migrate the relevant parts of your BLE
logic to an ESP‑IDF application where the NimBLE host headers are available.

Quick steps:

- Install ESP‑IDF and set up the environment per Espressif's docs.
- Use the example in `esp-idf-examples/gatts_set_attr_perm` as a starting point.
- Register your GATT services/characteristics using the NimBLE host APIs
  (`ble_gatt_svc_def`, `ble_gatts_register_svcs`, etc.). After registration,
  query the attribute handles and call `ble_gatts_set_attr_perm(handle, perm)`
  to set encryption/authentication requirements.

Notes:
- The Arduino core often exposes NimBLE's high-level C++ wrappers but omits
  some host-level helper functions in its static libraries; the host APIs are
  available in ESP‑IDF builds that include the NimBLE host implementation.
- Building an ESP‑IDF app gives you deterministic access to these symbols.
- If you prefer, only migrate the small bonding/permission portion to ESP‑IDF
  and keep the rest of your firmware logic in the Arduino sketch; however,
  mixing runtimes is non-trivial and generally it's simpler to port the whole
  firmware to ESP‑IDF when deterministic host-level control is required.
