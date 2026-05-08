#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

typedef enum {
    CBOR_TYPE_UINT64,
    CBOR_TYPE_INT64,
    CBOR_TYPE_BOOLEAN,
    CBOR_TYPE_RAW,
    CBOR_TYPE_TEXT_STRING,
    CBOR_TYPE_BYTE_STRING,

    CBOR_TYPE_ARRAY,
    CBOR_TYPE_MAP
} cbor_value_type_t;

typedef struct {
    size_t size;
    cbor_value_type_t type;

    union {
        uint8_t  *raw;   // Byte-Array
        uint32_t *u32;   // Unsigned 32-bit
        uint64_t *u64;   // Unsigned 64-bit
        int32_t  *i32;   // Signed 32-bit
        int64_t  *i64;   // Signed 64-bit
        bool      *boolean; // Boolean
    } value;
} cbor_property_t;

typedef struct cbor_helper_head_t {
    cbor_property_t property;
    struct cbor_helper_head_t *next;
} cbor_helper_head_t;

#define CBOR_BUFFER_SIZE 256

size_t cbor_helper_return_uint32(uint8_t *buffer, size_t buffer_size, uint32_t key, uint32_t value);
size_t cbor_helper_return_boolean(uint8_t *buffer, size_t buffer_size, uint32_t key, bool value);
size_t cbor_helper_return_text_string(uint8_t *buffer, size_t buffer_size, uint32_t key, const char *value, size_t value_len);
size_t cbor_helper_return_uint8_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint8_t *value, size_t value_len);
size_t cbor_helper_return_uint16_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint16_t *value, size_t value_len);
size_t cbor_helper_return_uint32_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint32_t *value, size_t value_len);
size_t cbor_helper_return_uint64_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint64_t *value, size_t value_len);

// esp_err_t cbor_helper_parse_item... only internal
size_t cbor_helper_encode(uint8_t *buffer, size_t buffer_size, cbor_helper_head_t *head);
cbor_helper_head_t* cbor_helper_parse(const uint8_t *cbor_data, size_t cbor_data_len);
void cbor_helper_print(cbor_helper_head_t *head, uint8_t indent_level);

cbor_helper_head_t* cbor_helper_add_uint64(cbor_helper_head_t **head, uint64_t value);
cbor_helper_head_t* cbor_helper_add_text_string(cbor_helper_head_t **head, const char *value);
cbor_helper_head_t* cbor_helper_get_element_at(cbor_helper_head_t *head, uint16_t index);
cbor_helper_head_t* cbor_helper_get_array(cbor_helper_head_t *head, uint16_t key);
esp_err_t cbor_helper_get_uint64(cbor_helper_head_t *head, uint16_t key, uint64_t *value);
esp_err_t cbor_helper_get_int64(cbor_helper_head_t *head, uint16_t key, int64_t *value);
esp_err_t cbor_helper_get_boolean(cbor_helper_head_t *head, uint16_t key, bool *value);
esp_err_t cbor_helper_get_text_string(cbor_helper_head_t *head, uint16_t key, char *value, size_t value_len);
void cbor_helper_free(cbor_helper_head_t *head);