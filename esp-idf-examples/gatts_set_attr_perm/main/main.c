#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

// NimBLE / host headers
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "host/ble_hs.h"
#include "host/ble_store.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "gatts_perm_example";

/* A tiny helper to be called after the GATT service/characteristic are created */
static void set_permission_example(uint16_t attr_handle) {
    ESP_LOGI(TAG, "Setting permission on attr handle %u", (unsigned)attr_handle);
    /* BLE_GATT_PERM_* macros are provided by NimBLE host headers */
    int rc = ble_gatts_set_attr_perm(attr_handle, BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED);
    if (rc == 0) {
        ESP_LOGI(TAG, "ble_gatts_set_attr_perm succeeded");
    } else {
        ESP_LOGE(TAG, "ble_gatts_set_attr_perm failed rc=%d", rc);
    }
}

static void ble_app_on_sync(void) {
    /* Create a simple GATT service with one characteristic.
       In a real app you'd call the full GATT registration helpers. Here we
       rely on the lower-level host API to show how to set perms. */
    ESP_LOGI(TAG, "ble synced - you can now register services and set perms from host APIs");
    /* In a typical flow you'd create service/characteristic via ble_gatt_svc_def
       and after registration query characteristic handles; once you have a
       specific attribute handle, call set_permission_example(handle).
       For brevity we omit the full GATT registration here. */
}

static void ble_host_task(void *param) {
    ESP_LOGI(TAG, "Starting NimBLE host task");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP‑IDF NimBLE GATT perm example");
    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    /* Start the host task */
    xTaskCreate(ble_host_task, "ble_host", 4096, NULL, 5, NULL);

    /* In a real example: register services, find the correct attr handle and
       then call set_permission_example(handle). See README for notes. */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
