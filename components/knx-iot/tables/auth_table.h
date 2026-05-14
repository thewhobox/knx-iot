#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    COAP_DTLS = 1,
    COAP_OSCORE = 2,
    COAP_TLS = 254,
    COAP_PASE = 255
} auth_profile_t;


typedef struct auth_entry_t {
    uint8_t *id;
    size_t id_len;
    auth_profile_t profile;

    uint8_t *key_id;
    size_t key_id_len;
    uint8_t *key_id_context;
    size_t key_id_context_len;
    uint8_t * master_secret;
    size_t master_secret_len;

    uint32_t *scopes;
    size_t scope_count;
    struct auth_entry_t *next;
} auth_entry_t;

esp_err_t auth_table_add_entry(auth_entry_t *entry);
esp_err_t auth_table_remove_entry(uint16_t id);
esp_err_t auth_table_load();
esp_err_t auth_table_save();
void auth_table_clear();
void auth_table_print();
void auth_table_init();