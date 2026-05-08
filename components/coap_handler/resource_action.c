#include "resource_action.h"

#include "esp_log.h"

#include "cbor_helper.h"

static const char *TAG = "CoAP Handler<action>";

static void resource_action_lsm_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /a/lsm");

	uint8_t *buffer = heap_caps_malloc(100, MALLOC_CAP_SPIRAM);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	size_t len = cbor_helper_return_uint32(buffer, sizeof(buffer), 3, 0);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	heap_caps_free(buffer);
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
	coap_add_resource(ctx, resource);
}