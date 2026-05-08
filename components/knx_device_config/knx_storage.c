#include "knx_storage.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "KNX_STORAGE";

esp_err_t knx_storage_set_u16(const char *ns, const char *key, uint16_t value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_set_u16(nvs_handle, key, value);
        if (err == ESP_OK) {
            err = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
    }
    return err;
}

esp_err_t knx_storage_set_u64(const char *ns, const char *key, uint64_t value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_set_u64(nvs_handle, key, value);
        if (err == ESP_OK) {
            err = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
    }
    return err;
}

esp_err_t knx_storage_set_blob(const char *ns, const char *key, const void *value, size_t length) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs_handle, key, value, length);
        if (err == ESP_OK) {
            err = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
    }
    return err;
}

esp_err_t knx_storage_get_u16(const char *ns, const char *key, uint16_t *value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_get_u16(nvs_handle, key, value);
        nvs_close(nvs_handle);
    }
    return err;
}

esp_err_t knx_storage_get_u64(const char *ns, const char *key, uint64_t *value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_get_u64(nvs_handle, key, value);
        nvs_close(nvs_handle);
    }
    return err;
}

esp_err_t knx_storage_get_blob(const char *ns, const char *key, void *value, size_t *length) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_get_blob(nvs_handle, key, value, length);
        nvs_close(nvs_handle);
    }
    return err;
}

void knx_storage_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS Flash init failed with %s, erasing and retrying...", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
}