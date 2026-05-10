#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef struct repu_entry_t {
    uint16_t id;
    uint16_t individual_address;
    uint32_t *group_addresses;
    size_t group_addresses_count;
    uint32_t group_id;
    struct repu_entry_t *next;
} repu_entry_t;

esp_err_t repu_table_add_recipient(repu_entry_t *entry);
esp_err_t repu_table_add_publisher(repu_entry_t *entry);

void repu_table_save_recipient();
void repu_table_save_publisher();

esp_err_t repu_table_load_recipient();
esp_err_t repu_table_load_publisher();
void repu_table_print_recipient();
void repu_table_print_publisher();

void repu_table_init();