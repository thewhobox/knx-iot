#include "knx_device_lsm.h"

#include "esp_log.h"

static const char* TAG = "KNX LSM";

lsm_state_t knx_device_current_state = UNLOADED;

lsm_state_t knx_device_lsm_get_state() {
    return knx_device_current_state;
}

void knx_device_lsm_handle_event(lsm_event_t event) {
    ESP_LOGI(TAG, "Handling event: %d in state: %d", event, knx_device_current_state);

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
}