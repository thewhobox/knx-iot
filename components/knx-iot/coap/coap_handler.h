#pragma once

#include "coap_config.h"
#include "coap3/coap.h"

#ifndef CONFIG_COAP_SERVER_SUPPORT
#error COAP_SERVER_SUPPORT needs to be enabled
#endif /* COAP_SERVER_SUPPORT */


void coap_handler_init();