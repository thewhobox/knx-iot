#include "repu_table.h"

#include <string.h>
#include "esp_log.h"
#include "knx_storage.h"

#define REPU_TABLE_SAVE_VERSION 1

static const char *TAG = "Repu Table";

repu_entry_t *recipient_table_head = NULL;
repu_entry_t *publisher_table_head = NULL;

esp_err_t repu_table_add_recipient(repu_entry_t *entry)
{
    ESP_LOGI(TAG, "Adding recipient entry");
    if(entry == NULL) {
        ESP_LOGE(TAG, "repu_table_add_recipient: entry is null");
        return ESP_FAIL;
    }

    // Check if entry with same id already exists
    repu_entry_t *current = recipient_table_head;
    while (current) {
        if(current->id == entry->id) {
            ESP_LOGE(TAG, "repu_table_add_recipient: Entry with id %u already exists in recipient table", entry->id);
            return ESP_FAIL;
        }
        current = current->next;
    }

    // Add new entry at the beginning of the list
    entry->next = recipient_table_head;
    recipient_table_head = entry;

    return ESP_OK;
}

esp_err_t repu_table_add_publisher(repu_entry_t *entry)
{
    ESP_LOGI(TAG, "Adding publisher entry");
    if(entry == NULL) {
        ESP_LOGE(TAG, "repu_table_add_publisher: entry is null");
        return ESP_FAIL;
    }

    // Check if entry with same id already exists
    repu_entry_t *current = publisher_table_head;
    while (current) {
        if(current->id == entry->id) {
            ESP_LOGE(TAG, "repu_table_add_publisher: Entry with id %u already exists in publisher table", entry->id);
            return ESP_FAIL;
        }
        current = current->next;
    }

    // Add new entry at the beginning of the list
    entry->next = publisher_table_head;
    publisher_table_head = entry;

    return ESP_OK;
}

static esp_err_t repu_table_save_table(repu_entry_t *head, const char *key)
{
    size_t buffer_len = 1; // version byte
    repu_entry_t *current = head;
    while (current) {
        buffer_len += 2; // id
        buffer_len += 2; // individual_address
        buffer_len += 4; // group_id
        buffer_len += 1; // group_addresses_count
        buffer_len += current->group_addresses_count * 4;
        current = current->next;
    }

    uint8_t *buffer = KNX_MALLOC(buffer_len);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "repu_table_save_table: Failed to allocate memory");
        return ESP_ERR_NO_MEM;
    }

    buffer[0] = REPU_TABLE_SAVE_VERSION;
    uint16_t offset = 1;
    current = head;
    while (current) {
        memcpy(buffer + offset, &current->id, 2);                offset += 2;
        memcpy(buffer + offset, &current->individual_address, 2); offset += 2;
        memcpy(buffer + offset, &current->group_id, 4);          offset += 4;
        buffer[offset] = (uint8_t)current->group_addresses_count; offset += 1;
        memcpy(buffer + offset, current->group_addresses, current->group_addresses_count * 4);
        offset += current->group_addresses_count * 4;
        current = current->next;
    }

    if(offset != buffer_len) {
        ESP_LOGE(TAG, "repu_table_save_table: Calculated buffer length does not match actual data length for key %s (offset=%u, buffer_len=%u)", key, offset, (unsigned)buffer_len);
        KNX_FREE(buffer);
        return ESP_FAIL;
    }

    knx_storage_set_blob("device", key, buffer, buffer_len);
    KNX_FREE(buffer);
    return ESP_OK;
}

static esp_err_t repu_table_load_table(repu_entry_t **head, const char *key)
{
    uint8_t *buffer = NULL;
    size_t buffer_len = 0;
    esp_err_t err = knx_storage_get_blob("device", key, NULL, &buffer_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "repu_table_load_table: No table found in storage for key %s", key);
        return ESP_OK;
    }
    if (buffer_len == 0) {
        ESP_LOGW(TAG, "repu_table_load_table: Empty blob for key %s", key);
        return ESP_OK;
    }
    buffer = malloc(buffer_len);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "repu_table_load_table: Failed to allocate memory");
        return ESP_ERR_NO_MEM;
    }
    err = knx_storage_get_blob("device", key, buffer, &buffer_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "repu_table_load_table: Failed to load from storage: %d", err);
        free(buffer);
        return err;
    }
    if (buffer[0] != REPU_TABLE_SAVE_VERSION) {
        ESP_LOGE(TAG, "repu_table_load_table: Unsupported version %u for key %s", buffer[0], key);
        free(buffer);
        return ESP_FAIL;
    }

    uint16_t offset = 1;
    while (offset < buffer_len) {
        repu_entry_t *entry = KNX_MALLOC(sizeof(repu_entry_t));
        if (entry == NULL) {
            ESP_LOGE(TAG, "repu_table_load_table: Failed to allocate entry");
            free(buffer);
            break;
        }
        memcpy(&entry->id, buffer + offset, 2);                  offset += 2;
        memcpy(&entry->individual_address, buffer + offset, 2);  offset += 2;
        memcpy(&entry->group_id, buffer + offset, 4);            offset += 4;
        entry->group_addresses_count = buffer[offset];           offset += 1;
        entry->group_addresses = KNX_MALLOC(sizeof(uint32_t) * entry->group_addresses_count);
        if (entry->group_addresses == NULL) {
            ESP_LOGE(TAG, "repu_table_load_table: Failed to allocate group addresses");
            KNX_FREE(entry);
            free(buffer);
            break;
        }
        memcpy(entry->group_addresses, buffer + offset, entry->group_addresses_count * 4);
        offset += entry->group_addresses_count * 4;
        entry->next = *head;
        *head = entry;
    }
    
    if(offset != buffer_len) {
        ESP_LOGE(TAG, "repu_table_load_table: Buffer length mismatch after loading table for key %s (offset=%u, buffer_len=%u)", key, offset, (unsigned)buffer_len);
    }

    free(buffer);
    return ESP_OK;
}

void repu_table_save(bool is_recipient)
{
    if (is_recipient) {
        repu_table_save_table(recipient_table_head, "fp/r");
    } else {
        repu_table_save_table(publisher_table_head, "fp/p");
    }
}

void repu_table_save_recipient()
{
    repu_table_save(true);
}

void repu_table_save_publisher()
{
    repu_table_save(false);
}

esp_err_t repu_table_load_recipient()
{
    return repu_table_load_table(&recipient_table_head, "fp/r");
}

esp_err_t repu_table_load_publisher()
{
    return repu_table_load_table(&publisher_table_head, "fp/p");
}

void repu_table_print_recipient()
{
    if(recipient_table_head == NULL) {
        ESP_LOGI(TAG, "Recipient table is empty");
        return;
    }
    repu_entry_t *current = recipient_table_head;
    while (current) {
		ESP_LOGI(TAG, "Recipient entry: id=%u, gid=%u, addrs=%u", current->id, current->group_id, current->group_addresses_count);
		ESP_LOG_BUFFER_HEX("Group Addresses", current->group_addresses, sizeof(uint32_t) * current->group_addresses_count);
        current = current->next;
    }
}

void repu_table_print_publisher()
{
    if(publisher_table_head == NULL) {
        ESP_LOGI(TAG, "Publisher table is empty");
        return;
    }
    repu_entry_t *current = publisher_table_head;
    while (current) {
		ESP_LOGI(TAG, "Publisher entry: id=%u, gid=%u, addrs=%u", current->id, current->group_id, current->group_addresses_count);
		ESP_LOG_BUFFER_HEX("Group Addresses", current->group_addresses, sizeof(uint32_t) * current->group_addresses_count);
        current = current->next;
    }
}

void repu_table_init()
{
    if(repu_table_load_recipient() != ESP_OK) {
        ESP_LOGE(TAG, "repu_table_init: Failed to load recipient table from storage");
    } else {
        repu_table_print_recipient();
    }

    if(repu_table_load_publisher() != ESP_OK) {
        ESP_LOGE(TAG, "repu_table_init: Failed to load publisher table from storage");
    } else {
        repu_table_print_publisher();
    }
}