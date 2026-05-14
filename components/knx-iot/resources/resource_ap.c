#include "resource_ap.h"

#include "esp_log.h"

#include "cbor_helper.h"
#include "knx_device_config.h"
#include "knx_storage.h"

static const char *TAG = "CoAP Handler<ap>";

static void resource_ap_pv_put_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "PUT request received for /ap/pv");

	uint8_t buffer_size = 100;
	uint8_t *buffer = KNX_MALLOC(buffer_size);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "resource_ap_pv_put_handler: Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "resource_ap_pv_put_handler: Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

	ESP_LOGI(TAG, "resource_ap_pv_put_handler: Got Array of length: %u", (unsigned)cbor_data->property.size);

	cbor_helper_print(cbor_data, 1);

	cbor_helper_head_t *cbor_pv = cbor_helper_get_array(cbor_data, 1);
	if(cbor_pv == NULL) {
		ESP_LOGE(TAG, "resource_ap_pv_put_handler: Failed to get array with key 1 from CBOR data");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
		cbor_helper_free(cbor_data);
		return;
	}

	if(cbor_pv->property.size != 3)
	{
		ESP_LOGE(TAG, "resource_ap_pv_put_handler: Array with key 1 has invalid length %u (expected 3)", (unsigned)cbor_pv->property.size);
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
		cbor_helper_free(cbor_data);
		return;
	}

	uint16_t pv[3];
	for(int i = 0; i < 3; i++)
	{
		cbor_helper_head_t *item = cbor_helper_get_element_at(cbor_pv, i);
		if(item == NULL) {
			ESP_LOGE(TAG, "resource_ap_pv_put_handler: Failed to get element at index %u from array with key 1", i);
			coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
			cbor_helper_free(cbor_data);
			return;
		}
		if(item->property.type != CBOR_TYPE_UINT64) {
			ESP_LOGE(TAG, "resource_ap_pv_put_handler: Element at index %u from array with key 1 is not an unsigned integer", i);
			coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
			cbor_helper_free(cbor_data);
			return;
		}
		pv[i] = *item->property.value.u16;
	}

	knx_device_config_set_application_version(pv);

	cbor_helper_free(cbor_data);

	// unsigned char buf[3];
	// coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	// coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

void resource_ap_init(coap_context_t *ctx)
{
    coap_resource_t *resource = coap_resource_init(coap_make_str_const("ap/pv"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() failed ap/pv");
		coap_free_context(ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_PUT, resource_ap_pv_put_handler);
	coap_add_resource(ctx, resource);
}