file(GLOB SOURCES "*.c")

idf_component_register(SRCS ${SOURCES}
                    REQUIRES mdns esp_wifi nvs_flash esp_driver_gpio coap_handler knx_device_config
                    INCLUDE_DIRS ".")