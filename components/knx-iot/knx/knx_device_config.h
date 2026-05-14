#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void knx_device_config_get_hardware_type(uint8_t **hardware_type, size_t *len);
void knx_device_config_get_hardware_version(uint8_t **hardware_version, size_t *len);
void knx_device_config_get_firmware_version(uint8_t **firmware_version, size_t *len);
void knx_device_config_get_model(uint8_t **model, size_t *len);
uint16_t knx_device_config_get_manufacturer_id();
uint16_t knx_device_config_get_individual_address();
void knx_device_config_set_individual_address(uint16_t individual_address);
uint64_t knx_device_config_get_installation_id();
void knx_device_config_set_installation_id(uint64_t installation_id);
uint16_t *knx_device_config_get_application_version();
void knx_device_config_set_application_version(uint16_t *data);
uint8_t *knx_device_config_get_serialnumber();
void knx_device_set_prog_mode(bool progmode);
bool knx_device_get_prog_mode();
void knx_device_config_init();