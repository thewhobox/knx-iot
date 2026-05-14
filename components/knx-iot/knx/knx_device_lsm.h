#pragma once

#include <stdint.h>

typedef enum {
    UNLOADED = 0,
    LOADED = 1,
    LOADING = 2,
    UNLOADING = 4,
    LOADCOMPLETING = 5
} lsm_state_t;

typedef enum class {
    NO_OPERATION = 0,
    STARTLOADING = 1,
    LOADCOMPLETE = 2,
    UNLOAD = 4
} lsm_event_t;

lsm_state_t knx_device_lsm_get_state();
void knx_device_lsm_handle_event(lsm_event_t event);
void knx_device_lsm_init();