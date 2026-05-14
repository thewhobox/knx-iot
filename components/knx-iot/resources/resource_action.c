#include "resource_action.h"

#include "esp_log.h"

#include "cbor_helper.h"
#include "knx_device_lsm.h"
#include "knx_storage.h"

static const char *TAG = "CoAP Handler<action>";

static void resource_action_lsm_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /a/lsm");

	uint8_t buffer_size = 100;
	uint8_t *buffer = KNX_MALLOC(buffer_size);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint32_t lsm_state = knx_device_lsm_get_state();
	size_t len = cbor_helper_return_uint32(buffer, buffer_size, 3, lsm_state);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_action_lsm_post_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "POST request received for /a/lsm");

	uint8_t buffer_size = 100;
	uint8_t *buffer = KNX_MALLOC(buffer_size);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

    uint64_t lsm_event = 0;
    // read individual address (key 12) and installation ID (key 26) from parsed CBOR data
    if(cbor_helper_get_uint64(cbor_data, 2, &lsm_event) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get lsm event from parsed CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        KNX_FREE(cbor_data);
        return;
    }
	ESP_LOGI(TAG, "LSM event: %llu", lsm_event);
	KNX_FREE(cbor_data);
	knx_device_lsm_handle_event(lsm_event);

	uint32_t lsm_state = knx_device_lsm_get_state();
	size_t len = cbor_helper_return_uint32(buffer, buffer_size, 3, lsm_state);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

void resource_action_init(coap_context_t *ctx)
{
    coap_resource_t *resource = coap_resource_init(coap_make_str_const("a/lsm"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() failed");
		coap_free_context(ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_action_lsm_get_handler);
	coap_register_handler(resource, COAP_REQUEST_POST, resource_action_lsm_post_handler);
	coap_add_resource(ctx, resource);
}