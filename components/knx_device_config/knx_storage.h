#pragma once

#include "esp_heap_caps.h"
#include <stdint.h>
#include "esp_err.h"

#define KNX_MALLOC(X)  heap_caps_malloc(X, MALLOC_CAP_SPIRAM);
#define KNX_FREE(X)    heap_caps_free(X);

esp_err_t knx_storage_set_u16(const char *ns, const char *key, uint16_t value);
esp_err_t knx_storage_set_u64(const char *ns, const char *key, uint64_t value);
esp_err_t knx_storage_set_blob(const char *ns, const char *key, const void *value, size_t length);
esp_err_t knx_storage_get_u16(const char *ns, const char *key, uint16_t *value);
esp_err_t knx_storage_get_u64(const char *ns, const char *key, uint64_t *value);
esp_err_t knx_storage_get_blob(const char *ns, const char *key, void *value, size_t *length);
void knx_storage_init();