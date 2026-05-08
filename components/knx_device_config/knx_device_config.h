#pragma once

#include <stdint.h>

uint64_t knx_device_config_get_installation_id();
uint16_t knx_device_config_get_individual_address();
uint8_t *knx_device_config_get_serialnumber();
void knx_device_config_init();