idf_component_register(SRCS "main.c"
                    REQUIRES mdns esp_wifi nvs_flash coap esp_driver_gpio
                    INCLUDE_DIRS ".")