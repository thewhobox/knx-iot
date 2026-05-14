#include "knx_iot.h"

#include "coap_handler.h"
#include "knx_device_config.h"
#include "tables/group_object_table.h"
#include "tables/repu_table.h"
#include "tables/auth_table.h"

void knx_iot_init() {
    knx_device_config_init();
    group_object_table_init();
    repu_table_init();
    auth_table_init();
    
    coap_handler_init();
}