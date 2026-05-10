#include "tables/group_object_table.h"

#include "esp_log.h"
#include "knx_storage.h"

static const char *TAG = "Group Object Table";

group_object_entry_t *group_object_table_head = NULL;


esp_err_t group_object_table_add_entry(group_object_entry_t *entry)
{
    if(entry == NULL) {
        ESP_LOGE(TAG, "group_object_table_add_entry: entry is null");
        return ESP_FAIL;
    }

    // Check if entry with same id already exists
    group_object_entry_t *current = group_object_table_head;
    while (current) {
        if(current->id == entry->id) {
            ESP_LOGE(TAG, "group_object_table_add_entry: Entry with id %u already exists in group object table", entry->id);
            return ESP_FAIL;
        }
        current = current->next;
    }

    // Add new entry at the beginning of the list
    entry->next = group_object_table_head;
    group_object_table_head = entry;

    return ESP_OK;
}

esp_err_t group_object_table_remove_entry(uint16_t id)
{
    if(group_object_table_head == NULL) {
        ESP_LOGE(TAG, "group_object_table_remove_entry: group object table is empty");
        return ESP_FAIL;
    }

    group_object_entry_t *current = group_object_table_head;
    group_object_entry_t *previous = NULL;

    while (current) {
        if(current->id == id) {
            if(previous) {
                previous->next = current->next;
            } else {
                group_object_table_head = current->next;
            }
            KNX_FREE(current);
            return ESP_OK;
        }
        previous = current;
        current = current->next;
    }

    ESP_LOGE(TAG, "group_object_table_remove_entry: Entry with id %u not found", id);
    return ESP_FAIL;
}

esp_err_t group_object_table_load()
{
    uint8_t *buffer = NULL;
    size_t buffer_len = 0;
    esp_err_t err = knx_storage_get_blob("device", "fp/g", NULL, &buffer_len);
    if(err != ESP_OK) {
        ESP_LOGW(TAG, "group_object_table_load: No group object table found in storage");
        return ESP_OK;
    }
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "group_object_table_load: Failed to get blob size from storage: %d", err);
        return err;
    }
    if(buffer_len == 0) {
        ESP_LOGW(TAG, "group_object_table_load: No group object table found in storage");
        return ESP_OK;
    }
    buffer = malloc(buffer_len);
    if(buffer == NULL) {
        ESP_LOGE(TAG, "group_object_table_load: Failed to allocate memory for buffer");
        return ESP_ERR_NO_MEM;
    }
    err = knx_storage_get_blob("device", "fp/g", buffer, &buffer_len);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "group_object_table_load: Failed to load group object table from storage: %d", err);
        free(buffer);
        return err;
    }
    if(buffer[0] != GROUP_OBJECT_TABLE_SAVE_VERSION) {
        ESP_LOGE(TAG, "group_object_table_load: Unsupported group object table version: %u", buffer[0]);
        free(buffer);
        return ESP_FAIL;
    }

    uint16_t offset = 1;
    while(offset < buffer_len) {
        group_object_entry_t *entry = KNX_MALLOC(sizeof(group_object_entry_t));
        if(entry == NULL) {
            ESP_LOGE(TAG, "group_object_table_load: Failed to allocate memory for group object entry");
            free(buffer);
            break;
        }

        entry->id = *(uint16_t *)(buffer + offset);
        offset += 2;

        uint8_t href_len = buffer[offset];
        offset += 1;

        memcpy(entry->href, buffer + offset, href_len);
        entry->href[href_len] = '\0';
        offset += href_len;

        entry->group_addresses_count = buffer[offset];
        offset += 1;
        
        entry->group_addresses = KNX_MALLOC(sizeof(uint32_t) * entry->group_addresses_count);
        if(entry->group_addresses == NULL) {
            ESP_LOGE(TAG, "group_object_table_load: Failed to allocate memory for group addresses");
            KNX_FREE(entry);
            free(buffer); // buffer is not allocated from KNX_MALLOC, so use free
            break;
        }
        memcpy(entry->group_addresses, buffer + offset, sizeof(uint32_t) * entry->group_addresses_count);
        offset += sizeof(uint32_t) * entry->group_addresses_count;

        entry->cflag.flags = buffer[offset];
        offset += 1;

        entry->next = group_object_table_head;
        group_object_table_head = entry;
    }

    if(offset - 1 != buffer_len) {
        ESP_LOGE(TAG, "group_object_table_load: Buffer length mismatch after loading group object table (offset=%u, buffer_len=%u)", offset, (unsigned)buffer_len);
    }
    free(buffer); // buffer is not allocated from KNX_MALLOC, so use free
    return ESP_OK;
}

esp_err_t group_object_table_save()
{
    uint8_t *buffer = NULL;
    size_t buffer_len = 1;
    group_object_entry_t *current = group_object_table_head;
    while (current) {
        // calculate needed buffer size for current entry
        buffer_len += 2; // id
        buffer_len += 1 + strlen(current->href); // href
        buffer_len += 1 + current->group_addresses_count * 4; // group addresses
        buffer_len += 1; // cflags
        current = current->next;
    }

    buffer = KNX_MALLOC(buffer_len + 1);
    if(buffer == NULL) {
        ESP_LOGE(TAG, "group_object_table_save: Failed to allocate memory for buffer");
        return ESP_ERR_NO_MEM;
    }

    buffer[0] = GROUP_OBJECT_TABLE_SAVE_VERSION; // version

    current = group_object_table_head;
    uint16_t offset = 1;
    while (current) {
        memcpy(buffer + offset, &current->id, 2);
        offset += 2;
        buffer[offset] = strlen(current->href);
        offset += 1;
        memcpy(buffer + offset, current->href, strlen(current->href));
        offset += strlen(current->href);
        buffer[offset] = current->group_addresses_count;
        offset += 1;
        memcpy(buffer + offset, current->group_addresses, current->group_addresses_count * 4);
        offset += current->group_addresses_count * 4;
        memcpy(buffer + offset, &current->cflag, 1);
        offset += 1;
        current = current->next;
    }

    if(offset - 1 != buffer_len) {
        ESP_LOGE(TAG, "group_object_table_save: Calculated buffer length does not match actual data length (offset=%u, buffer_len=%u)", offset, (unsigned)buffer_len);
        KNX_FREE(buffer);
        return ESP_FAIL;
    }

    knx_storage_set_blob("device", "fp/g", buffer, buffer_len);

    KNX_FREE(buffer);
    return ESP_OK;
}

void group_object_table_clear()
{
    group_object_entry_t *current = group_object_table_head;
    while (current) {
        group_object_entry_t *next = current->next;
        KNX_FREE(current->group_addresses);
        KNX_FREE(current);
        current = next;
    }
    group_object_table_head = NULL;
}

void group_object_table_print()
{
    group_object_entry_t *current = group_object_table_head;
    while (current) {
		ESP_LOGI(TAG, "Parsed entry: id=%u, cflags=%02x, href=%s addrs=%u", current->id, current->cflag.flags, current->href, current->group_addresses_count);
		ESP_LOG_BUFFER_HEX("Group Addresses", current->group_addresses, sizeof(uint32_t) * current->group_addresses_count);
        current = current->next;
    }
}

void group_object_table_init()
{
    if(group_object_table_load() != ESP_OK) {
        ESP_LOGE(TAG, "group_object_table_init: Failed to load group object table from storage");
    } else {
        group_object_table_print();
    }
}