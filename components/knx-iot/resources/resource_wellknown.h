#pragma once

#include "coap_config.h"
#include "coap3/coap.h"

/* Keys for .well-known/knx */
#define CBOR_KEY_WK_ERASE_CODE 1
/* Keys for .well-known/knx/ia */
#define CBOR_KEY_WKIA_INDIVIDUAL_ADDRESS 12
#define CBOR_KEY_WKIA_INSTALLATION_ID 26


void resource_wellknown_init(coap_context_t *ctx);