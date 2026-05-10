#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#define GROUP_OBJECT_HREF_MAX_LEN 255
#define GROUP_OBJECT_TABLE_SAVE_VERSION 1

typedef struct group_object_entry_t {
    uint16_t id;
    char href[GROUP_OBJECT_HREF_MAX_LEN];
    uint32_t *group_addresses;
    size_t group_addresses_count;
    union {
        uint8_t flags;
        struct {
            uint8_t reserved : 2;
            uint8_t read     : 1;
            uint8_t write    : 1;
            uint8_t init     : 1;
            uint8_t transmit : 1;
            uint8_t update   : 1;
            uint8_t unused   : 1;
        } bits;
    } cflag;
    struct group_object_entry_t *next;
} group_object_entry_t;

esp_err_t group_object_table_add_entry(group_object_entry_t *entry);
esp_err_t group_object_table_remove_entry(uint16_t id);
esp_err_t group_object_table_load();
esp_err_t group_object_table_save();
void group_object_table_clear();
void group_object_table_print();
void group_object_table_init();