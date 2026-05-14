#include "knx_device_config.h"

#include <string.h>
#include "esp_mac.h"
#include "esp_log.h"
#include "knx_storage.h"
#include "knx_device_lsm.h"

static const char *TAG = "KNX_DEVICE_CONFIG";

uint8_t device_serial[6] = {0x00, 0xFA, 0x10, 0x02, 0x07, 0x01}; // → 00:FA:10:02:70:01
uint16_t device_individual_address = 0xFFFF; // → FFFF (broadcast address)
uint64_t device_installation_id = 0;
bool device_prog_mode = false;
uint8_t device_hardware_type[12] = "000102030405";
uint8_t device_hardware_version[3] = {1, 0, 0};
uint8_t device_firmware_version[3] = {2, 0, 0};
uint8_t device_model[8] = "IoT Demo";
uint16_t device_application_version[3] = {250, 0, 0};

void knx_device_config_get_hardware_type(uint8_t **hardware_type, size_t *len)
{
    *hardware_type = device_hardware_type;
    *len = sizeof(device_hardware_type);
}

void knx_device_config_get_hardware_version(uint8_t **hardware_version, size_t *len)
{
    *hardware_version = device_hardware_version;
    *len = sizeof(device_hardware_version);
}

void knx_device_config_get_firmware_version(uint8_t **firmware_version, size_t *len)
{
    *firmware_version = device_firmware_version;
    *len = sizeof(device_firmware_version);
}

void knx_device_config_get_model(uint8_t **model, size_t *len)
{
    *model = device_model;
    *len = sizeof(device_model);
}

uint16_t knx_device_config_get_manufacturer_id()
{
    return (device_serial[0] << 8) | device_serial[1];
}

void knx_device_config_set_individual_address(uint16_t individual_address)
{
    device_individual_address = individual_address;
    esp_err_t err = knx_storage_set_u16("device", "dev_ia", individual_address);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save individual address: %s", esp_err_to_name(err));
    }
}

void knx_device_config_set_installation_id(uint64_t installation_id)
{
    device_installation_id = installation_id;
    esp_err_t err = knx_storage_set_u64("device", "dev_iia", installation_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save installation id: %s", esp_err_to_name(err));
    }
}

uint64_t knx_device_config_get_installation_id()
{
    return device_installation_id;
}

uint16_t knx_device_config_get_individual_address()
{
    return device_individual_address;
}

uint16_t *knx_device_config_get_application_version()
{
    return device_application_version;
}

void knx_device_config_set_application_version(uint16_t *data)
{
    memcpy(device_application_version, data, sizeof(device_application_version));
    esp_err_t err = knx_storage_set_blob("device", "app_version", (uint8_t *)device_application_version, sizeof(device_application_version));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save application version: %s", esp_err_to_name(err));
    }
}

uint8_t *knx_device_config_get_serialnumber()
{
    return device_serial;
}

void knx_device_set_prog_mode(bool progmode)
{
    device_prog_mode = progmode;
}

bool knx_device_get_prog_mode()
{
    return device_prog_mode;
}

void knx_device_config_init()
{
    // For demonstration purposes, we use a fixed serial number and installation ID.
    // In a real application, you would likely want to generate these based on the device's unique identifiers (e.g., MAC address).
    // You could also store them in non-volatile storage and load them on startup.

    knx_storage_init(); // Initialize storage (e.g., NVS)

    // Example: Generate serial number based on MAC address
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    memcpy(device_serial, mac, 6);
    device_serial[0] = 0x00;
    device_serial[1] = 0xFA; // Set Manufacturer ID to 00:FA (example)
    ESP_LOGI(TAG, "Device Serialnumber:       %02X%02X%02X%02X%02X%02X",
        device_serial[0], device_serial[1], device_serial[2], device_serial[3], device_serial[4], device_serial[5]);

    // NVS laden
    uint16_t ia = 0xFFFF;
    uint64_t iia = 0;
    if (knx_storage_get_u16("device", "dev_ia", &ia) == ESP_OK) {
        device_individual_address = ia;
    }
    if (knx_storage_get_u64("device", "dev_iia", &iia) == ESP_OK) {
        device_installation_id = iia;
    }

    uint8_t *app_version = KNX_MALLOC(sizeof(device_application_version));
    if(app_version == NULL) {
        ESP_LOGE(TAG, "Failed to load application version from storage: Out of memory");
    } else {
        size_t app_version_len = sizeof(device_application_version);
        esp_err_t err = knx_storage_get_blob("device", "app_version", app_version, &app_version_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load application version from storage: %s", esp_err_to_name(err));
        } else if(app_version_len != sizeof(device_application_version)) {
            ESP_LOGE(TAG, "Failed to load application version from storage: Invalid size");
        } else {
            memcpy(device_application_version, app_version, sizeof(device_application_version));
        }
    }

    ESP_LOGI(TAG, "Loaded Individual Address: %u.%u.%u", (device_individual_address >> 12) & 0xF, (device_individual_address >> 8) & 0xF, device_individual_address & 0xFF);
    ESP_LOGI(TAG, "Loaded Installation ID:    %" PRIx64, device_installation_id);
    ESP_LOGI(TAG, "Loaded Application Version: %.4X - %.4X - %.4X", device_application_version[0], device_application_version[1], device_application_version[2]);
    
    knx_device_lsm_init();

    lsm_state_t lsm_state = knx_device_lsm_get_state();
    switch(lsm_state)
    {
        case UNLOADED:
            ESP_LOGI(TAG, "LSM State: UNLOADED");
            break;
        case LOADED:
            ESP_LOGI(TAG, "LSM State: LOADED");
            break;
        case LOADING:
            ESP_LOGI(TAG, "LSM State: LOADING");
            break;
        case UNLOADING:
            ESP_LOGI(TAG, "LSM State: UNLOADING");
            break;
        case LOADCOMPLETING:
            ESP_LOGI(TAG, "LSM State: LOADCOMPLETING");
            break;
        default:
            ESP_LOGI(TAG, "LSM State: UNKNOWN");
    }
}