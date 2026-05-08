#include "knx_device_config.h"

#include <string.h>
#include "esp_mac.h"

uint8_t device_serial[6] = {0x00, 0xFA, 0x10, 0x02, 0x07, 0x01}; // → 00:FA:10:02:70:01
uint16_t device_individual_address = 0xFFFF; // → FFFF (broadcast address)
uint64_t device_installation_id = 0;

uint64_t knx_device_config_get_installation_id()
{
    return device_installation_id;
}

uint16_t knx_device_config_get_individual_address()
{
    return device_individual_address;
}

uint8_t *knx_device_config_get_serialnumber()
{
    return device_serial;
}

void knx_device_config_init()
{
    // For demonstration purposes, we use a fixed serial number and installation ID.
    // In a real application, you would likely want to generate these based on the device's unique identifiers (e.g., MAC address).
    // You could also store them in non-volatile storage and load them on startup.

    // Example: Generate serial number based on MAC address
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    // memcpy(device_serial, mac, 6);
    printf("Device ID: %02X%02X%02X%02X%02X%02X\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}