#include "cbor_helper.h"

#include "knx_storage.h"

#include "cbor.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "CBOR_HELPER";

size_t cbor_helper_return_uint32(uint8_t *buffer, size_t buffer_size, uint32_t key, uint32_t value)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);
    cbor_encode_uint(&mapEncoder, value);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

size_t cbor_helper_return_boolean(uint8_t *buffer, size_t buffer_size, uint32_t key, bool value)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);
    cbor_encode_boolean(&mapEncoder, value);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

size_t cbor_helper_return_text_string(uint8_t *buffer, size_t buffer_size, uint32_t key, const char *value, size_t value_len)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);
    cbor_encode_text_string(&mapEncoder, value, value_len);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

size_t cbor_helper_return_uint8_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint8_t *value, size_t value_len)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;
    CborEncoder arrayEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);

    cbor_encoder_create_array(&mapEncoder, &arrayEncoder, value_len);
    for(int i = 0; i < value_len; i++) {
        cbor_encode_uint(&arrayEncoder, value[i]);
    }
    cbor_encoder_close_container(&mapEncoder, &arrayEncoder);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

size_t cbor_helper_return_uint16_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint16_t *value, size_t value_len)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;
    CborEncoder arrayEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);

    cbor_encoder_create_array(&mapEncoder, &arrayEncoder, value_len);
    for(int i = 0; i < value_len; i++) {
        cbor_encode_uint(&arrayEncoder, value[i]);
    }
    cbor_encoder_close_container(&mapEncoder, &arrayEncoder);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

size_t cbor_helper_return_uint32_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint32_t *value, size_t value_len)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;
    CborEncoder arrayEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);

    cbor_encoder_create_array(&mapEncoder, &arrayEncoder, value_len);
    for(int i = 0; i < value_len; i++) {
        cbor_encode_uint(&arrayEncoder, value[i]);
    }
    cbor_encoder_close_container(&mapEncoder, &arrayEncoder);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

size_t cbor_helper_return_uint64_array(uint8_t *buffer, size_t buffer_size, uint32_t key, uint64_t *value, size_t value_len)
{
    CborEncoder encoder;
    CborEncoder mapEncoder;
    CborEncoder arrayEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);
    // Eine Map mit 1 Eintrag starten
    cbor_encoder_create_map(&encoder, &mapEncoder, 1);
    // Key = key, Value = value
    cbor_encode_uint(&mapEncoder, key);

    cbor_encoder_create_array(&mapEncoder, &arrayEncoder, value_len);
    for(int i = 0; i < value_len; i++) {
        cbor_encode_uint(&arrayEncoder, value[i]);
    }
    cbor_encoder_close_container(&mapEncoder, &arrayEncoder);
    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

esp_err_t cbor_helper_parse_item(CborValue *it, cbor_helper_head_t **parent)
{
    cbor_helper_head_t *item = KNX_MALLOC(sizeof(cbor_helper_head_t));
    if (!item) {
        ESP_LOGE(TAG, "cbor_helper_parse_item: Memory allocation failed");
        return ESP_FAIL;
    }
    memset(item, 0, sizeof(cbor_helper_head_t));
    item->next = NULL;

    CborError err = CborNoError;

    if (cbor_value_is_unsigned_integer(it)) {
        uint64_t v;
        err = cbor_value_get_uint64(it, &v);
        if(err != CborNoError)
        {
            ESP_LOGE(TAG, "Failed to read unsigned integer: %d", err);
            return ESP_FAIL;
        }
        item->property.type = CBOR_TYPE_UINT64;
        item->property.value.u64 = KNX_MALLOC(sizeof(uint64_t));
        if (!item->property.value.u64) {
            ESP_LOGE(TAG, "Memory allocation failed");
            return ESP_FAIL;
        }
        *(item->property.value.u64) = v;
    }
    else if (cbor_value_is_integer(it)) {
        int64_t v;
        err = cbor_value_get_int64(it, &v);
        if(err != CborNoError)
        {
            ESP_LOGE(TAG, "Failed to read signed integer: %d", err);
            return ESP_FAIL;
        }
        item->property.type = CBOR_TYPE_INT64;
        item->property.value.i64 = KNX_MALLOC(sizeof(int64_t));
        if (!item->property.value.i64) {
            ESP_LOGE(TAG, "Memory allocation failed");
            return ESP_FAIL;
        }
        *(item->property.value.i64) = v;
    }
    else if(cbor_value_is_boolean(it))
    {
        bool v;
        err = cbor_value_get_boolean(it, &v);
        if(err != CborNoError)
        {
            ESP_LOGE(TAG, "Failed to read boolean: %d", err);
            return ESP_FAIL;
        }
        item->property.type = CBOR_TYPE_BOOLEAN;
        item->property.value.boolean = KNX_MALLOC(sizeof(bool));
        if (!item->property.value.boolean) {
            ESP_LOGE(TAG, "Memory allocation failed");
            return ESP_FAIL;
        }
        *(item->property.value.boolean) = v;
    }
    else if (cbor_value_is_byte_string(it)) {
        uint8_t buf[256];
        size_t len = sizeof(buf);

        err = cbor_value_copy_byte_string(it, buf, &len, NULL);
        if (err != CborNoError) {
            ESP_LOGE(TAG, "Failed to get byte string: %d", err);
            return ESP_FAIL;
        }
        
        item->property.type = CBOR_TYPE_BYTE_STRING;
        item->property.value.raw = KNX_MALLOC(len);
        if (!item->property.value.raw) {
            ESP_LOGE(TAG, "Memory allocation failed");
            return ESP_FAIL;
        }
        memcpy(item->property.value.raw, buf, len);
        item->property.size = len;
    }
    else if (cbor_value_is_text_string(it)) {
        char buf[256];
        size_t len = sizeof(buf);

        err = cbor_value_copy_text_string(it, buf, &len, NULL);
        if (err != CborNoError) {
            ESP_LOGE(TAG, "Failed to get text string: %d", err);
            return ESP_FAIL;
        }

        item->property.type = CBOR_TYPE_TEXT_STRING;
        item->property.value.raw = KNX_MALLOC(len+1);
        if (!item->property.value.raw) {
            ESP_LOGE(TAG, "Memory allocation failed");
            return ESP_FAIL;
        }
        memcpy(item->property.value.raw, buf, len);
        item->property.value.raw[len] = '\0';
        item->property.size = len;
    }
    else if (cbor_value_is_array(it)) {
        item->property.type = CBOR_TYPE_ARRAY;

        CborError err_len = cbor_value_get_array_length(it, &(item->property.size));
        if (err_len != CborNoError) {
            ESP_LOGE(TAG, "Failed to get array length: %d", err_len);
            return ESP_FAIL;
        }

        // TODO also allow array with 0 length
        if (item->property.size == 0) {
            return ESP_FAIL;
        }

        CborValue arr;
        esp_err_t err_enter = cbor_value_enter_container(it, &arr);
        if (err_enter != CborNoError) {
            ESP_LOGE(TAG, "Failed to enter array: %d", err_enter);
            return ESP_FAIL;
        }
        
        cbor_helper_head_t *current = NULL;
        while (!cbor_value_at_end(&arr) && arr.type != CborInvalidType)
        {
            cbor_helper_parse_item(&arr, &current);

            if(current->next != NULL)
                current = current->next;
            else
                item->property.value.raw = (uint8_t *)current;
        }
        cbor_value_leave_container(it, &arr);
    }
    else if (cbor_value_is_map(it)) {
        item->property.type = CBOR_TYPE_MAP;

        err = cbor_value_get_map_length(it, &(item->property.size));
        if (err != CborNoError) {
            ESP_LOGE(TAG, "Failed to get map length: %d", err);
            return ESP_FAIL;
        }

        CborValue map;
        err = cbor_value_enter_container(it, &map);
        if (err != CborNoError) {
            ESP_LOGE(TAG, "Failed to enter map: %d", err);
            return ESP_FAIL;
        }

        cbor_helper_head_t *current = NULL;
        while (!cbor_value_at_end(&map))
        {
            cbor_helper_parse_item(&map, &current);

            if(current->next != NULL)
                current = current->next;
            else
                item->property.value.raw = (uint8_t *)current;
        }
        cbor_value_leave_container(it, &map);
    }
    else {
        ESP_LOGW("CBOR", "Unsupported CBOR type");
        err = CborErrorUnsupportedType;
    }

    // Für skalare Typen Iterator vorrücken; Container werden bereits durch
    // cbor_value_leave_container() weitergesetzt.
    if (item->property.type != CBOR_TYPE_ARRAY && item->property.type != CBOR_TYPE_MAP) {
        cbor_value_advance(it);
    }

    if(err != CborNoError)
    {
        ESP_LOGE(TAG, "cbor_helper_parse_item: Failed to parse CBOR item: %d", err);
        KNX_FREE(item);
        return ESP_FAIL;
    }

    if(*parent == NULL)
    {
        *parent = item;
    } else {
        (*parent)->next = item;
    }

    return ESP_OK;
}

size_t cbor_helper_encode(uint8_t *buffer, size_t buffer_size, cbor_helper_head_t *head)
{
    
    CborEncoder encoder;
    CborEncoder mapEncoder;

    // Encoder initialisieren
    cbor_encoder_init(&encoder, buffer, buffer_size, 0);

    uint16_t item_counter = 0;
    cbor_helper_head_t *current = head;
    while(current) {
        item_counter++;
        current = current->next;
    }
    // Eine Map mit item_counter Einträgen starten
    cbor_encoder_create_map(&encoder, &mapEncoder, item_counter / 2);

    current = head;
    while(current) {
        ESP_LOGI(TAG, "Encoding item with type %u", (uint16_t)current->property.type);
        if(current->property.type == CBOR_TYPE_UINT64) {
            ESP_LOGI(TAG, "Encoding uint64 with value %" PRIu64, *(current->property.value.u64));
            cbor_encode_uint(&mapEncoder, *(current->property.value.u64));
        }
        else if(current->property.type == CBOR_TYPE_INT64) {
            ESP_LOGI(TAG, "Encoding int64 with value %" PRIi64, *(current->property.value.i64));
            cbor_encode_int(&mapEncoder, *(current->property.value.i64));
        }
        else if(current->property.type == CBOR_TYPE_BOOLEAN) {
            ESP_LOGI(TAG, "Encoding boolean with value %s", *(current->property.value.boolean) ? "true" : "false");
            cbor_encode_boolean(&mapEncoder, *(current->property.value.boolean));
        }
        else if(current->property.type == CBOR_TYPE_TEXT_STRING) {
            ESP_LOGI(TAG, "Encoding text string: %s", (char*)current->property.value.raw);
            cbor_encode_text_string(&mapEncoder, (char*)current->property.value.raw, current->property.size);
        }
        else if(current->property.type == CBOR_TYPE_BYTE_STRING) {
            ESP_LOGI(TAG, "Encoding byte string (%u bytes)", (unsigned)current->property.size);
            ESP_LOG_BUFFER_HEX(TAG, current->property.value.raw, current->property.size);
            cbor_encode_byte_string(&mapEncoder, current->property.value.raw, current->property.size);
        }

        current = current->next;
    }

    // Map schließen
    cbor_encoder_close_container(&encoder, &mapEncoder);
    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

cbor_helper_head_t* cbor_helper_parse(const uint8_t *cbor_data, size_t cbor_data_len)
{
    CborParser parser;
    CborValue it;

    CborError err = cbor_parser_init(cbor_data, cbor_data_len, 0, &parser, &it);
    if (err != CborNoError) {
        ESP_LOGE(TAG, "Parser init failed: %d", err);
        return NULL;
    }

    cbor_helper_head_t *head = NULL;
    cbor_helper_parse_item(&it, &head);

    return head;
}

void cbor_helper_print(cbor_helper_head_t *head, uint8_t indent_level)
{
    cbor_helper_head_t *current = head;
    while(current) {
        if(current->property.value.raw == NULL) {
            ESP_LOGI(TAG, "%*sNULL value with type %u", indent_level * 2, "", (uint16_t)current->property.type);
            current = current->next;
            continue;
        }
        if(current->property.type == CBOR_TYPE_UINT64) {
            ESP_LOGI(TAG, "%*suint64: %" PRIu64, indent_level * 2, "", *(current->property.value.u64));
        }
        else if(current->property.type == CBOR_TYPE_INT64) {
            ESP_LOGI(TAG, "%*sint64: %" PRIi64, indent_level * 2, "", *(current->property.value.i64));
        }
        else if(current->property.type == CBOR_TYPE_BOOLEAN) {
            ESP_LOGI(TAG, "%*sboolean: %s", indent_level * 2, "", *(current->property.value.boolean) ? "true" : "false");
        }
        else if(current->property.type == CBOR_TYPE_TEXT_STRING) {
            ESP_LOGI(TAG, "%*stext string: %s", indent_level * 2, "", (char*)current->property.value.raw);
        }
        else if(current->property.type == CBOR_TYPE_BYTE_STRING) {
            ESP_LOGI(TAG, "%*sbyte string (%u bytes):", indent_level * 2, "", (unsigned)current->property.size);
            ESP_LOG_BUFFER_HEX(TAG, current->property.value.raw, current->property.size);
        }
        else if(current->property.type == CBOR_TYPE_ARRAY) {
            ESP_LOGI(TAG, "%*sArray with %u items:", indent_level * 2, "", (unsigned)current->property.size);
            cbor_helper_print((cbor_helper_head_t *)current->property.value.raw, indent_level + 1);
        }
        else if(current->property.type == CBOR_TYPE_MAP) {
            ESP_LOGI(TAG, "%*sMap with %u pairs:", indent_level * 2, "", (unsigned)current->property.size);
            cbor_helper_print((cbor_helper_head_t *)current->property.value.raw, indent_level + 1);
        }

        current = current->next;
    }
}

cbor_helper_head_t* cbor_helper_add_uint64(cbor_helper_head_t **head, uint64_t value)
{
    cbor_helper_head_t *item = KNX_MALLOC(sizeof(cbor_helper_head_t));
    if (!item) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return *head;
    }
    memset(item, 0, sizeof(cbor_helper_head_t));
    item->property.type = CBOR_TYPE_UINT64;
    item->property.value.u64 = KNX_MALLOC(sizeof(uint64_t));
    if (!item->property.value.u64) {
        ESP_LOGE(TAG, "Memory allocation failed");
        KNX_FREE(item);
        return *head;
    }
    *(item->property.value.u64) = value;
    if(*head != NULL)
        (*head)->next = item;
    else
        *head = item;
    return item;
}

cbor_helper_head_t* cbor_helper_add_text_string(cbor_helper_head_t **head, const char *value)
{
    cbor_helper_head_t *item = KNX_MALLOC(sizeof(cbor_helper_head_t));
    if (!item) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return *head;
    }
    memset(item, 0, sizeof(cbor_helper_head_t));
    item->property.type = CBOR_TYPE_TEXT_STRING;
    item->property.value.raw = KNX_MALLOC(strlen(value) + 1);
    if (!item->property.value.raw) {
        ESP_LOGE(TAG, "Memory allocation failed");
        KNX_FREE(item);
        return *head;
    }
    memcpy(item->property.value.raw, value, strlen(value));
    item->property.value.raw[strlen(value)] = '\0';
    item->property.size = strlen(value);
    if(*head != NULL)
        (*head)->next = item;
    else
        *head = item;
    return item;
}

cbor_helper_head_t* cbor_helper_get_element_at(cbor_helper_head_t *head, uint16_t index)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_element_at: CBOR head is NULL");
        return NULL;
    }
    if(head->property.type != CBOR_TYPE_ARRAY) {
        ESP_LOGE(TAG, "cbor_helper_get_element_at: CBOR data is not an array");
        return NULL;
    }

    if(index >= head->property.size) {
        ESP_LOGE(TAG, "cbor_helper_get_element_at: Index %u out of bounds (array size is %u)", index, (unsigned)head->property.size);
        return NULL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    uint16_t current_index = 0;
    while (current) {
        if(current_index == index)
            return current;
        // skip next item
        current = current->next;
        current_index++;
    }

    ESP_LOGE(TAG, "cbor_helper_get_element_at: Element with index %u not found", index);
    return NULL;
}

cbor_helper_head_t* cbor_helper_get_map(cbor_helper_head_t *head, uint16_t key)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_map: CBOR head is NULL");
        return NULL;
    }
    if(head->property.type != CBOR_TYPE_MAP) {
        ESP_LOGE(TAG, "cbor_helper_get_map: CBOR data is not a map");
        return NULL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    while (current) {
        if (current->property.type == CBOR_TYPE_UINT64 || current->property.type == CBOR_TYPE_INT64)
        {
            if(*current->property.value.u64 == key)
            {
                cbor_helper_head_t *value_item = current->next;
                if(value_item == NULL) {
                    ESP_LOGE(TAG, "cbor_helper_get_map: Property with key %u has no value", key);
                    return NULL;
                }
                
                if (value_item->property.type == CBOR_TYPE_MAP) {
                    return value_item;
                } else {
                    ESP_LOGW(TAG, "cbor_helper_get_map: Property with key %u is not a map", key);
                    return NULL;
                }
            }
        } else {
            ESP_LOGE(TAG, "cbor_helper_get_map: Property key is not an integer. wanted key=%u, type is=%u)", key, current->property.type);
            return NULL;
        }
        if(current->next == NULL) {
            ESP_LOGE(TAG, "cbor_helper_get_map: Property with key %" PRIi64 " has no value", current->property.value.u64);
            return NULL;
        }
        // skip next item (key + value)
        current = current->next->next;
    }
    ESP_LOGW(TAG, "cbor_helper_get_map: Property with key %u not found", key);
    return NULL;
}

cbor_helper_head_t* cbor_helper_get_array(cbor_helper_head_t *head, uint16_t key)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_array: CBOR head is NULL");
        return NULL;
    }
    if(head->property.type != CBOR_TYPE_MAP) {
        ESP_LOGE(TAG, "cbor_helper_get_array: CBOR data is not a map");
        return NULL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    while (current) {
        if (current->property.type == CBOR_TYPE_UINT64 || current->property.type == CBOR_TYPE_INT64)
        {
            if(*current->property.value.u64 == key)
            {
                cbor_helper_head_t *value_item = current->next;
                if(value_item == NULL) {
                    ESP_LOGE(TAG, "cbor_helper_get_array: Property with key %u has no value", key);
                    return NULL;
                }
                
                if (value_item->property.type == CBOR_TYPE_ARRAY) {
                    return value_item;
                } else {
                    ESP_LOGW(TAG, "cbor_helper_get_array: Property with key %u is not an array", key);
                    return NULL;
                }
            }
        } else {
            ESP_LOGE(TAG, "cbor_helper_get_array: Property key is not an integer. wanted key=%u, type is=%u)", key, current->property.type);
            return NULL;
        }
        if(current->next == NULL) {
            ESP_LOGE(TAG, "cbor_helper_get_array: Property with key %" PRIi64 " has no value", current->property.value.u64);
            return NULL;
        }
        // skip next item (key + value)
        current = current->next->next;
    }
    ESP_LOGW(TAG, "cbor_helper_get_array: Property with key %u not found", key);
    return NULL;
}

esp_err_t cbor_helper_get_uint64(cbor_helper_head_t *head, uint16_t key, uint64_t *value)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_uint64: CBOR head is NULL");
        return ESP_FAIL;
    }
    if(head->property.type != CBOR_TYPE_MAP) {
        ESP_LOGE(TAG, "cbor_helper_get_uint64: CBOR data is not a map");
        return ESP_FAIL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    while (current) {
        if (current->property.type == CBOR_TYPE_UINT64 || current->property.type == CBOR_TYPE_INT64)
        {
            if(*current->property.value.u64 == key)
            {
                cbor_helper_head_t *value_item = current->next;
                if(value_item == NULL) {
                    ESP_LOGE(TAG, "cbor_helper_get_uint64: Property with key %u has no value", key);
                    return ESP_FAIL;
                }
                
                if (value_item->property.type == CBOR_TYPE_UINT64) {
                    *value = *(value_item->property.value.u64);
                    return ESP_OK;
                } else {
                    ESP_LOGW(TAG, "cbor_helper_get_uint64: Property with key %u is not a uint64", key);
                    return ESP_FAIL;
                }
            }
        } else {
            ESP_LOGE(TAG, "cbor_helper_get_uint64: Property key is not an integer. wanted key=%u, type is=%u)", key, current->property.type);
            return ESP_FAIL;
        }
        if(current->next == NULL) {
            ESP_LOGE(TAG, "cbor_helper_get_uint64: Property with key %" PRIi64 " has no value", current->property.value.u64);
            return ESP_FAIL;
        }
        // skip next item (key + value)
        current = current->next->next;
    }
    ESP_LOGW(TAG, "cbor_helper_get_uint64: Property with key %u not found", key);
    return ESP_FAIL;
}

esp_err_t cbor_helper_get_int64(cbor_helper_head_t *head, uint16_t key, int64_t *value)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_int64: CBOR head is NULL");
        return ESP_FAIL;
    }
    if(head->property.type != CBOR_TYPE_MAP) {
        ESP_LOGE(TAG, "cbor_helper_get_int64: CBOR data is not a map");
        return ESP_FAIL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    while (current) {
        if (current->property.type == CBOR_TYPE_UINT64 || current->property.type == CBOR_TYPE_INT64)
        {
            if(*current->property.value.u64 == key)
            {
                cbor_helper_head_t *value_item = current->next;
                if(value_item == NULL) {
                    ESP_LOGE(TAG, "cbor_helper_get_int64: Property with key %u has no value", key);
                    return ESP_FAIL;
                }
                
                if (value_item->property.type == CBOR_TYPE_INT64) {
                    *value = *(value_item->property.value.i64);
                    return ESP_OK;
                } else {
                    ESP_LOGW(TAG, "cbor_helper_get_int64: Property with key %u is not an int64", key);
                    return ESP_FAIL;
                }
            }
        } else {
            ESP_LOGE(TAG, "cbor_helper_get_int64: Property key is not an integer. wanted key=%u, type is=%u)", key, current->property.type);
            return ESP_FAIL;
        }
        if(current->next == NULL) {
            ESP_LOGE(TAG, "cbor_helper_get_int64: Property with key %" PRIi64 " has no value", current->property.value.u64);
            return ESP_FAIL;
        }
        // skip next item (key + value)
        current = current->next->next;
    }
    ESP_LOGW(TAG, "cbor_helper_get_int64: Property with key %u not found", key);
    return ESP_FAIL;
}

esp_err_t cbor_helper_get_boolean(cbor_helper_head_t *head, uint16_t key, bool *value)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_boolean: CBOR head is NULL");
        return ESP_FAIL;
    }
    if(head->property.type != CBOR_TYPE_MAP) {
        ESP_LOGE(TAG, "cbor_helper_get_boolean: CBOR data is not a map");
        return ESP_FAIL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    while (current) {
        if (current->property.type == CBOR_TYPE_UINT64 || current->property.type == CBOR_TYPE_INT64)
        {
            if(*current->property.value.u64 == key)
            {
                cbor_helper_head_t *value_item = current->next;
                if(value_item == NULL) {
                    ESP_LOGE(TAG, "cbor_helper_get_boolean: Property with key %u has no value", key);
                    return ESP_FAIL;
                }
                
                if (value_item->property.type == CBOR_TYPE_BOOLEAN) {
                    *value = *(value_item->property.value.boolean);
                    return ESP_OK;
                } else {
                    ESP_LOGW(TAG, "cbor_helper_get_boolean: Property with key %u is not a boolean", key);
                    return ESP_FAIL;
                }
            }
        } else {
            ESP_LOGE(TAG, "cbor_helper_get_boolean: Property key is not an integer. wanted key=%u, type is=%u)", key, current->property.type);
            return ESP_FAIL;
        }
        if(current->next == NULL) {
            ESP_LOGE(TAG, "cbor_helper_get_boolean: Property with key %" PRIi64 " has no value", current->property.value.u64);
            return ESP_FAIL;
        }
        // skip next item (key + value)
        current = current->next->next;
    }
    ESP_LOGW(TAG, "cbor_helper_get_boolean: Property with key %u not found", key);
    return ESP_FAIL;
}

esp_err_t cbor_helper_get_text_string(cbor_helper_head_t *head, uint16_t key, uint8_t **value, size_t *value_len)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_text_string: CBOR head is NULL");
        return ESP_FAIL;
    }
    if(head->property.type != CBOR_TYPE_MAP) {
        ESP_LOGE(TAG, "cbor_helper_get_text_string: CBOR data is not a map");
        return ESP_FAIL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    while (current) {
        if (current->property.type == CBOR_TYPE_UINT64 || current->property.type == CBOR_TYPE_INT64)
        {
            if(*current->property.value.u64 == key)
            {
                cbor_helper_head_t *value_item = current->next;
                if(value_item == NULL) {
                    ESP_LOGE(TAG, "cbor_helper_get_text_string: Property with key %u has no value", key);
                    return ESP_FAIL;
                }
                
                if (value_item->property.type == CBOR_TYPE_TEXT_STRING) {
                    *value = KNX_MALLOC(value_item->property.size);
                    if(*value == NULL) {
                        ESP_LOGE(TAG, "cbor_helper_get_text_string: Memory allocation failed");
                        return ESP_FAIL;
                    }
                    memcpy(*value, value_item->property.value.raw, value_item->property.size);
                    *value_len = value_item->property.size;
                    return ESP_OK;
                } else {
                    ESP_LOGW(TAG, "cbor_helper_get_text_string: Property with key %u is not a text string", key);
                    return ESP_FAIL;
                }
            }
        } else {
            ESP_LOGE(TAG, "cbor_helper_get_text_string: Property key is not an integer. wanted key=%u, type is=%u)", key, current->property.type);
            return ESP_FAIL;
        }
        if(current->next == NULL) {
            ESP_LOGE(TAG, "cbor_helper_get_text_string: Property with key %" PRIi64 " has no value", current->property.value.u64);
            return ESP_FAIL;
        }
        // skip next item (key + value)
        current = current->next->next;
    }
    ESP_LOGW(TAG, "cbor_helper_get_text_string: Property with key %u not found", key);
    return ESP_FAIL;
}

esp_err_t cbor_helper_get_byte_string(cbor_helper_head_t *head, uint16_t key, uint8_t **value, size_t *value_len)
{
    if(head == NULL) {
        ESP_LOGE(TAG, "cbor_helper_get_byte_string: CBOR head is NULL");
        return ESP_FAIL;
    }
    if(head->property.type != CBOR_TYPE_MAP) {
        ESP_LOGE(TAG, "cbor_helper_get_byte_string: CBOR data is not a map");
        return ESP_FAIL;
    }

    cbor_helper_head_t *current = (cbor_helper_head_t *)head->property.value.raw;
    while (current) {
        if (current->property.type == CBOR_TYPE_UINT64 || current->property.type == CBOR_TYPE_INT64)
        {
            if(*current->property.value.u64 == key)
            {
                cbor_helper_head_t *value_item = current->next;
                if(value_item == NULL) {
                    ESP_LOGE(TAG, "cbor_helper_get_byte_string: Property with key %u has no value", key);
                    return ESP_FAIL;
                }
                
                if (value_item->property.type == CBOR_TYPE_BYTE_STRING) {
                    *value = KNX_MALLOC(value_item->property.size);
                    if(*value == NULL) {
                        ESP_LOGE(TAG, "cbor_helper_get_byte_string: Memory allocation failed");
                        return ESP_FAIL;
                    }
                    memcpy(*value, value_item->property.value.raw, value_item->property.size);
                    *value_len = value_item->property.size;
                    return ESP_OK;
                } else {
                    ESP_LOGW(TAG, "cbor_helper_get_byte_string: Property with key %u is not a byte string", key);
                    return ESP_FAIL;
                }
            }
        } else {
            ESP_LOGE(TAG, "cbor_helper_get_byte_string: Property key is not an integer. wanted key=%u, type is=%u)", key, current->property.type);
            return ESP_FAIL;
        }
        if(current->next == NULL) {
            ESP_LOGE(TAG, "cbor_helper_get_byte_string: Property with key %" PRIi64 " has no value", current->property.value.u64);
            return ESP_FAIL;
        }
        // skip next item (key + value)
        current = current->next->next;
    }
    ESP_LOGW(TAG, "cbor_helper_get_byte_string: Property with key %u not found", key);
    return ESP_FAIL;
}

void cbor_helper_free(cbor_helper_head_t *head)
{
    cbor_helper_head_t *current = head;
    while (current) {
        if(current->property.type == CBOR_TYPE_MAP) {
            cbor_helper_free((cbor_helper_head_t *)current->property.value.raw);
            current->property.value.raw = NULL;
        }
        else if(current->property.type == CBOR_TYPE_ARRAY) {
            cbor_helper_free((cbor_helper_head_t *)current->property.value.raw);
            current->property.value.raw = NULL;
        }

        cbor_helper_head_t *next = current->next;
        if(current->property.value.raw)
        {
            KNX_FREE(current->property.value.raw);
            current->property.value.raw = NULL;
        }
        if(current)
        {
            KNX_FREE(current);
            current = NULL;
        }            
        current = next;
    }
}
