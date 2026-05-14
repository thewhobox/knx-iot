#include "auth_table.h"

#include <string.h>
#include "esp_log.h"
#include "knx_storage.h"

#define AUTH_TABLE_SAVE_VERSION 1

static const char *TAG = "Auth Table";

auth_entry_t *auth_table_head = NULL;


esp_err_t auth_table_add_entry(auth_entry_t *entry)
{
    if (entry == NULL) {
        ESP_LOGE(TAG, "auth_table_add_entry: entry is null");
        return ESP_FAIL;
    }

    // Check if entry with same id already exists
    auth_entry_t *current = auth_table_head;
    while (current) {
        if (current->id_len == entry->id_len && memcmp(current->id, entry->id, entry->id_len) == 0) {
            ESP_LOGE(TAG, "auth_table_add_entry: Entry with same id already exists in auth table");
            return ESP_FAIL;
        }
        current = current->next;
    }

    // Add new entry at the beginning of the list
    entry->next = auth_table_head;
    auth_table_head = entry;

    return ESP_OK;
}

esp_err_t auth_table_remove_entry(uint16_t id)
{
    if (auth_table_head == NULL) {
        ESP_LOGE(TAG, "auth_table_remove_entry: auth table is empty");
        return ESP_FAIL;
    }

    auth_entry_t *current = auth_table_head;
    auth_entry_t *previous = NULL;

    while (current) {
        if (*(uint16_t *)current->id == id) {
            if (previous) {
                previous->next = current->next;
            } else {
                auth_table_head = current->next;
            }
            KNX_FREE(current->id);
            KNX_FREE(current->key_id);
            KNX_FREE(current->key_id_context);
            KNX_FREE(current->master_secret);
            KNX_FREE(current->scopes);
            KNX_FREE(current);
            return ESP_OK;
        }
        previous = current;
        current = current->next;
    }

    ESP_LOGE(TAG, "auth_table_remove_entry: Entry with id %u not found", id);
    return ESP_FAIL;
}

esp_err_t auth_table_load()
{
    uint8_t *buffer = NULL;
    size_t buffer_len = 0;
    esp_err_t err = knx_storage_get_blob("device", "auth/at", NULL, &buffer_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "auth_table_load: No auth table found in storage");
        return ESP_OK;
    }
    if (buffer_len == 0) {
        ESP_LOGW(TAG, "auth_table_load: No auth table found in storage");
        return ESP_OK;
    }
    buffer = malloc(buffer_len);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "auth_table_load: Failed to allocate memory for buffer");
        return ESP_ERR_NO_MEM;
    }
    err = knx_storage_get_blob("device", "auth/at", buffer, &buffer_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "auth_table_load: Failed to load auth table from storage: %d", err);
        free(buffer);
        return err;
    }
    if (buffer[0] != AUTH_TABLE_SAVE_VERSION) {
        ESP_LOGE(TAG, "auth_table_load: Unsupported auth table version: %u", buffer[0]);
        free(buffer);
        return ESP_FAIL;
    }

    uint16_t offset = 1;
    while (offset < buffer_len) {
        auth_entry_t *entry = KNX_MALLOC(sizeof(auth_entry_t));
        if (entry == NULL) {
            ESP_LOGE(TAG, "auth_table_load: Failed to allocate memory for auth entry");
            free(buffer);
            break;
        }
        memset(entry, 0, sizeof(auth_entry_t));

        entry->id_len = buffer[offset];
        offset += 1;
        entry->id = KNX_MALLOC(entry->id_len);
        if (entry->id == NULL) {
            ESP_LOGE(TAG, "auth_table_load: Failed to allocate memory for id");
            KNX_FREE(entry);
            free(buffer);
            break;
        }
        memcpy(entry->id, buffer + offset, entry->id_len);
        offset += entry->id_len;

        entry->profile = (auth_profile_t)buffer[offset];
        offset += 1;

        entry->key_id_len = buffer[offset];
        offset += 1;
        entry->key_id = KNX_MALLOC(entry->key_id_len);
        if (entry->key_id == NULL) {
            ESP_LOGE(TAG, "auth_table_load: Failed to allocate memory for key_id");
            KNX_FREE(entry);
            free(buffer);
            break;
        }
        memcpy(entry->key_id, buffer + offset, entry->key_id_len);
        offset += entry->key_id_len;

        entry->key_id_context_len = buffer[offset];
        offset += 1;
        entry->key_id_context = KNX_MALLOC(entry->key_id_context_len);
        if (entry->key_id_context == NULL) {
            ESP_LOGE(TAG, "auth_table_load: Failed to allocate memory for key_id_context");
            KNX_FREE(entry->key_id);
            KNX_FREE(entry);
            free(buffer);
            break;
        }
        memcpy(entry->key_id_context, buffer + offset, entry->key_id_context_len);
        offset += entry->key_id_context_len;

        entry->master_secret_len = buffer[offset];
        offset += 1;
        entry->master_secret = KNX_MALLOC(entry->master_secret_len);
        if (entry->master_secret == NULL) {
            ESP_LOGE(TAG, "auth_table_load: Failed to allocate memory for master_secret");
            KNX_FREE(entry->key_id_context);
            KNX_FREE(entry->key_id);
            KNX_FREE(entry);
            free(buffer);
            break;
        }
        memcpy(entry->master_secret, buffer + offset, entry->master_secret_len);
        offset += entry->master_secret_len;

        entry->scope_count = buffer[offset];
        offset += 1;
        entry->scopes = KNX_MALLOC(sizeof(uint32_t) * entry->scope_count);
        if (entry->scopes == NULL) {
            ESP_LOGE(TAG, "auth_table_load: Failed to allocate memory for scopes");
            KNX_FREE(entry->master_secret);
            KNX_FREE(entry->key_id_context);
            KNX_FREE(entry->key_id);
            KNX_FREE(entry);
            free(buffer);
            break;
        }
        memcpy(entry->scopes, buffer + offset, sizeof(uint32_t) * entry->scope_count);
        offset += sizeof(uint32_t) * entry->scope_count;

        entry->next = auth_table_head;
        auth_table_head = entry;
    }

    if (offset != buffer_len) {
        ESP_LOGE(TAG, "auth_table_load: Buffer length mismatch after loading auth table (offset=%u, buffer_len=%u)", offset, (unsigned)buffer_len);
    }
    free(buffer);
    return ESP_OK;
}

esp_err_t auth_table_save()
{
    size_t buffer_len = 1; // version byte
    auth_entry_t *current = auth_table_head;
    while (current) {
        buffer_len += 1 + current->id_len;       // id_len + id
        buffer_len += 1;                         // profile
        buffer_len += 1 + current->key_id_len;  // key_id
        buffer_len += 1 + current->key_id_context_len; // key_id_context
        buffer_len += 1 + current->master_secret_len;  // master_secret
        buffer_len += 1 + current->scope_count * sizeof(uint32_t); // scopes
        current = current->next;
    }

    uint8_t *buffer = KNX_MALLOC(buffer_len);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "auth_table_save: Failed to allocate memory for buffer");
        return ESP_ERR_NO_MEM;
    }

    buffer[0] = AUTH_TABLE_SAVE_VERSION;
    uint16_t offset = 1;
    current = auth_table_head;
    while (current) {
        buffer[offset] = (uint8_t)current->id_len;
        offset += 1;
        memcpy(buffer + offset, current->id, current->id_len);
        offset += current->id_len;

        buffer[offset] = (uint8_t)current->profile;
        offset += 1;

        buffer[offset] = (uint8_t)current->key_id_len;
        offset += 1;
        memcpy(buffer + offset, current->key_id, current->key_id_len);
        offset += current->key_id_len;

        buffer[offset] = (uint8_t)current->key_id_context_len;
        offset += 1;
        memcpy(buffer + offset, current->key_id_context, current->key_id_context_len);
        offset += current->key_id_context_len;

        buffer[offset] = (uint8_t)current->master_secret_len;
        offset += 1;
        memcpy(buffer + offset, current->master_secret, current->master_secret_len);
        offset += current->master_secret_len;

        buffer[offset] = (uint8_t)current->scope_count;
        offset += 1;
        memcpy(buffer + offset, current->scopes, current->scope_count * sizeof(uint32_t));
        offset += current->scope_count * sizeof(uint32_t);

        current = current->next;
    }

    if (offset != buffer_len) {
        ESP_LOGE(TAG, "auth_table_save: Calculated buffer length does not match actual data length (offset=%u, buffer_len=%u)", offset, (unsigned)buffer_len);
        KNX_FREE(buffer);
        return ESP_FAIL;
    }

    knx_storage_set_blob("device", "auth/at", buffer, buffer_len);

    KNX_FREE(buffer);
    return ESP_OK;
}

void auth_table_clear()
{
    auth_entry_t *current = auth_table_head;
    while (current) {
        auth_entry_t *next = current->next;
        KNX_FREE(current->id);
        KNX_FREE(current->key_id);
        KNX_FREE(current->key_id_context);
        KNX_FREE(current->master_secret);
        KNX_FREE(current->scopes);
        KNX_FREE(current);
        current = next;
    }
    auth_table_head = NULL;
}

void auth_table_print()
{
    auth_entry_t *current = auth_table_head;
    while (current) {
        ESP_LOGI(TAG, "Auth entry: profile=%u, key_id_len=%u, key_id_context_len=%u, master_secret_len=%u, scope_count=%u",
                 current->profile,
                 (unsigned)current->key_id_len,
                 (unsigned)current->key_id_context_len,
                 (unsigned)current->master_secret_len,
                 (unsigned)current->scope_count);
        ESP_LOG_BUFFER_HEX("Auth ID", current->id, current->id_len);
        ESP_LOG_BUFFER_HEX("Auth Key ID", current->key_id, current->key_id_len);
        ESP_LOG_BUFFER_HEX("Auth Key ID Context", current->key_id_context, current->key_id_context_len);
        ESP_LOG_BUFFER_HEX("Auth Master Secret", current->master_secret, current->master_secret_len);
        for (size_t i = 0; i < current->scope_count; i++) {
            ESP_LOGI(TAG, "Auth Scope[%u]: %u", (unsigned)i, current->scopes[i]);
        }
        current = current->next;
    }
}

void auth_table_init()
{
    if (auth_table_load() != ESP_OK) {
        ESP_LOGE(TAG, "auth_table_init: Failed to load auth table from storage");
    } else {
        auth_table_print();
    }
}