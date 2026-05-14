#include "knx_device_lsm.h"

#include "knx_storage.h"

#include "esp_log.h"

static const char* TAG = "KNX LSM";

lsm_state_t knx_device_current_state = UNLOADED;

lsm_state_t knx_device_lsm_get_state() {
    return knx_device_current_state;
}

void knx_device_lsm_handle_event(lsm_event_t event) {
    ESP_LOGI(TAG, "Handling event: %d in state: %d", event, knx_device_current_state);
    lsm_state_t old_state = knx_device_current_state;

    switch (event) {
        case NO_OPERATION:
            // No state change
            break;
        
        case STARTLOADING:
            knx_device_current_state = LOADING;
            break;

        case LOADCOMPLETE:
            if(knx_device_current_state == LOADING) {
                knx_device_current_state = LOADED;
            } else {
                ESP_LOGE(TAG, "Invalid state transition: LOADCOMPLETE event in state %d", knx_device_current_state);
                knx_device_current_state = UNLOADED; // Reset to a safe state
            }
            break;

        case UNLOAD:
            knx_device_current_state = UNLOADED;
            // TODO delete all loaded resources and reset device state
            break;

        default:
            // Handle invalid state
            break;
    }

    if(old_state != knx_device_current_state) {
        ESP_LOGI(TAG, "State changed from %d to %d", old_state, knx_device_current_state);
        // Store the new state in persistent storage
        esp_err_t err = knx_storage_set_u16("device", "lsm_state", (uint16_t)knx_device_current_state);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to store LSM state: %s", esp_err_to_name(err));
        }
    }
}

void knx_device_lsm_init() {
    // Initialize the state machine to a known state
    uint16_t stored_state = 0;
    if (knx_storage_get_u16("device", "lsm_state", &stored_state) == ESP_OK)
    {
        knx_device_current_state = (lsm_state_t)stored_state;
    }
}