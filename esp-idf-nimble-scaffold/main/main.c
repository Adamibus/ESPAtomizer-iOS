/*
  ESP-IDF NimBLE full example

  - Initializes NimBLE
  - Registers a primary service + one read/write characteristic
  - On characteristic registration, calls ble_gatts_set_attr_perm to require
    encryption for reads and writes (so iOS will prompt to pair)
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatts.h"
#include "host/ble_gatt.h"

static const char *TAG = "nimble_full";

static const char *device_name = "ESP-IDF-NimBLE-FULL";

/* Advertising */
static void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields rc=%d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "Advertising started");
    }
}

static void ble_app_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc) {
        ESP_LOGE(TAG, "ble_hs_util_ensure_addr rc=%d", rc);
        return;
    }

    ble_app_advertise();
}

/* --- GATT server setup --- */
#define GATT_SVC_UUID 0xABF0
#define GATT_CHR_UUID 0xABF1

static uint8_t g_chr_value[16] = { '0' };

static int
gatt_svr_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        ble_gatt_access_rsp_flat(ctxt, g_chr_value, 1);
        return 0;
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        {
            struct os_mbuf *om = ctxt->om;
            int len = om->om_len;
            if (len > 0) {
                /* copy up to buffer size */
                int tocopy = len > (int)sizeof(g_chr_value) ? sizeof(g_chr_value) : len;
                memcpy(g_chr_value, om->om_data, tocopy);
            }
            return 0;
        }
    default:
        return BLE_HS_EINVAL;
    }
}

static int
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_REGISTER_OP_CHR) {
        ESP_LOGI(TAG, "Registered characteristic; attr_handle=%u, def_handle=%u",
                 ctxt->chr.attr_handle, ctxt->chr.def_handle);

        uint16_t value_handle = ctxt->chr.attr_handle;
        uint8_t perm = BLE_GATT_PERM_READ | BLE_GATT_PERM_READ_ENCRYPTED |
                       BLE_GATT_PERM_WRITE | BLE_GATT_PERM_WRITE_ENCRYPTED;
        int rc = ble_gatts_set_attr_perm(value_handle, perm);
        if (rc == 0) {
            ESP_LOGI(TAG, "ble_gatts_set_attr_perm OK for handle=%u", value_handle);
        } else {
            ESP_LOGW(TAG, "ble_gatts_set_attr_perm rc=%d for handle=%u", rc, value_handle);
        }
    }
    return 0;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_SVC_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(GATT_CHR_UUID),
                .access_cb = gatt_svr_chr_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            { 0 }
        }
    },
    { 0 }
};

void nimble_task(void *param)
{
    ESP_LOGI(TAG, "Initializing NimBLE host...");

    nimble_port_init();

    /* Register GATT register callback */
    ble_gatt_register_cb(gatt_svr_register_cb, NULL);

    /* Add services */
    int rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "GATT services added");
    }

    ble_hs_cfg_reset();
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    ble_svc_gap_device_name_set(device_name);

    nimble_port_freertos_init(NULL);
}

void app_main(void)
{
    esp_err_t ret = esp_nimble_hci_and_controller_init();
    if (ret) {
        ESP_LOGE(TAG, "esp_nimble_hci_and_controller_init failed: %d", ret);
        return;
    }

    xTaskCreatePinnedToCore(nimble_task, "nimble", 8192, NULL, 5, NULL, tskNO_AFFINITY);

    ESP_LOGI(TAG, "ESP-IDF NimBLE full example started");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
