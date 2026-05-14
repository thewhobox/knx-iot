#include "resource_dev.h"

#include "coap3/coap_session_internal.h"

#include "esp_log.h"

#include "knx_storage.h"
#include "knx_device_config.h"
#include "coap_handler.h"
#include "cbor_helper.h"

static const char *TAG = "CoAP Handler<dev>";

static void resource_dev_pm_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/pm");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	bool prog_mode = knx_device_get_prog_mode();
	size_t len = cbor_helper_return_boolean(buffer, CBOR_BUFFER_SIZE, 1, prog_mode);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_pm_put_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "PUT request received for /dev/pm");
    
    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

    bool prog_mode = false;
    // read individual address (key 12) and installation ID (key 26) from parsed CBOR data
    if(cbor_helper_get_boolean(cbor_data, 1, &prog_mode) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get new individual address from parsed CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        KNX_FREE(cbor_data);
        return;
    }

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
	
    KNX_FREE(cbor_data);
}

static void resource_dev_da_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/da");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint16_t individual_address = knx_device_config_get_individual_address();
	size_t len = cbor_helper_return_uint32(buffer, CBOR_BUFFER_SIZE, 1, individual_address & 0xFF);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_sna_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/sna");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint16_t individual_address = knx_device_config_get_individual_address();
	size_t len = cbor_helper_return_uint32(buffer, CBOR_BUFFER_SIZE, 1, (individual_address >> 8) & 0xFF);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_hwt_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/hwt");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint8_t *hardware_type = NULL;
    size_t hardware_type_len = 0;
    knx_device_config_get_hardware_type(&hardware_type, &hardware_type_len);
	ESP_LOG_BUFFER_HEXDUMP(TAG, hardware_type, hardware_type_len, ESP_LOG_INFO);
	size_t len = cbor_helper_return_text_string(buffer, CBOR_BUFFER_SIZE, 1, (const char *)hardware_type, hardware_type_len);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_hwv_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/hwv");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint8_t *hardware_version = NULL;
    size_t hardware_version_len = 0;
    knx_device_config_get_hardware_version(&hardware_version, &hardware_version_len);
	ESP_LOG_BUFFER_HEXDUMP(TAG, hardware_version, hardware_version_len, ESP_LOG_INFO);
	size_t len = cbor_helper_return_uint8_array(buffer, CBOR_BUFFER_SIZE, 1, hardware_version, hardware_version_len);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_fwv_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/fwv");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint8_t *firmware_version = NULL;
    size_t firmware_version_len = 0;
    knx_device_config_get_firmware_version(&firmware_version, &firmware_version_len);
	ESP_LOG_BUFFER_HEXDUMP(TAG, firmware_version, firmware_version_len, ESP_LOG_INFO);
	size_t len = cbor_helper_return_uint8_array(buffer, CBOR_BUFFER_SIZE, 1, firmware_version, firmware_version_len);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_model_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/model");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint8_t *model = NULL;
    size_t model_len = 0;
    knx_device_config_get_model(&model, &model_len);
	ESP_LOG_BUFFER_HEXDUMP(TAG, model, model_len, ESP_LOG_INFO);
	size_t len = cbor_helper_return_text_string(buffer, CBOR_BUFFER_SIZE, 1, (const char *)model, model_len);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_port_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/port or /dev/mport");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	size_t len = cbor_helper_return_uint32(buffer, CBOR_BUFFER_SIZE, 1, 5683);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_hname_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/hname");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint8_t *serialnumber = knx_device_config_get_serialnumber();

	uint8_t payload[100];
	int payload_len = snprintf((char *)payload, sizeof(payload), "knx-%.2X%.2X%.2X%.2X%.2X%.2X.local",
		serialnumber[0], serialnumber[1], serialnumber[2], serialnumber[3], serialnumber[4], serialnumber[5]);
	size_t len = cbor_helper_return_text_string(buffer, CBOR_BUFFER_SIZE, 1, (const char *)payload, payload_len);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_dev_mid_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "GET request received for /dev/mid");
    
    uint8_t *buffer = KNX_MALLOC(CBOR_BUFFER_SIZE);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

	uint16_t manufacturer_id = knx_device_config_get_manufacturer_id();
	size_t len = cbor_helper_return_uint32(buffer, CBOR_BUFFER_SIZE, 1, manufacturer_id);

	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

void resource_dev_init(coap_context_t *ctx)
{
	coap_resource_t *resource = coap_resource_init(coap_make_str_const("dev/pm"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/pm failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_pm_get_handler);
	coap_register_handler(resource, COAP_REQUEST_PUT, resource_dev_pm_put_handler);
	coap_add_resource(ctx, resource);
    
	resource = coap_resource_init(coap_make_str_const("dev/da"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/da failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_da_get_handler);
	coap_add_resource(ctx, resource);
    
	resource = coap_resource_init(coap_make_str_const("dev/sna"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/sna failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_sna_get_handler);
	coap_add_resource(ctx, resource);
    
	resource = coap_resource_init(coap_make_str_const("dev/hwt"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/hwt failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_hwt_get_handler);
	coap_add_resource(ctx, resource);
    
	resource = coap_resource_init(coap_make_str_const("dev/hwv"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/hwv failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_hwv_get_handler);
	coap_add_resource(ctx, resource);
    
	resource = coap_resource_init(coap_make_str_const("dev/fwv"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/fwv failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_fwv_get_handler);
	coap_add_resource(ctx, resource);
    
	resource = coap_resource_init(coap_make_str_const("dev/model"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/model failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_model_get_handler);
	coap_add_resource(ctx, resource);

	resource = coap_resource_init(coap_make_str_const("dev/mport"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/mport failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_port_get_handler);
	coap_add_resource(ctx, resource);

	resource = coap_resource_init(coap_make_str_const("dev/port"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/port failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_port_get_handler);
	coap_add_resource(ctx, resource);

	resource = coap_resource_init(coap_make_str_const("dev/hname"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/hname failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_hname_get_handler);
	coap_add_resource(ctx, resource);

	resource = coap_resource_init(coap_make_str_const("dev/mid"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() /dev/mid failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_dev_mid_get_handler);
	coap_add_resource(ctx, resource);

	// TODO:
	// - dev/ipv6
}