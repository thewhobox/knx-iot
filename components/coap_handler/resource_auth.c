#include "resource_auth.h"

#include "esp_log.h"

#include "cbor_helper.h"
#include "knx_device_lsm.h"
#include "knx_storage.h"
#include "tables/group_object_table.h"
#include "tables/repu_table.h"

static const char *TAG = "CoAP Handler<auth>";

static void resource_fp_g_post_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "POST request received for /fp/g");

	uint8_t buffer_size = 100;
	uint8_t *buffer = KNX_MALLOC(buffer_size);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

	ESP_LOGI(TAG, "resource_fp_g_post_handler: Got Array of length: %u", (unsigned)cbor_data->property.size);

	cbor_helper_print(cbor_data, 1);

	for(int i = 0; i < cbor_data->property.size; i++)
	{
		cbor_helper_head_t *item = cbor_helper_get_element_at(cbor_data, i);
		if(item == NULL) {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to get element at index %u", i);
			continue;
		}

		group_object_entry_t *entry = KNX_MALLOC(sizeof(group_object_entry_t));
		if(entry == NULL) {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to allocate memory for group object entry");
			continue;
		}

		uint64_t temp_value = 0;
		// Get id
		if(cbor_helper_get_uint64(item, 0, &temp_value) == ESP_OK) {
			if(temp_value > UINT16_MAX) {
				ESP_LOGE(TAG, "resource_fp_g_post_handler: Value for id at index %u is greater than 0xFFFF large: %" PRIx64, i, temp_value);
				continue;
			}
			entry->id = (uint16_t)temp_value;
		}
		else {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to get id value from element at index %u", i);
		}

		// Get CFlags
		if(cbor_helper_get_uint64(item, 8, &temp_value) == ESP_OK) {
			if(temp_value & 3) {
				ESP_LOGE(TAG, "resource_fp_g_post_handler: Value has set bits in cflags which are reserved %" PRIx64, i, temp_value);
				continue;
			}
			entry->cflag.flags = (uint8_t)temp_value;
		}
		else {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to get cflags value from element at index %u", i);
		}

		// Get href
		if(cbor_helper_get_text_string(item, 11, entry->href, GROUP_OBJECT_HREF_MAX_LEN) != ESP_OK) {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to get href value from element at index %u", i);
		}

		// Get group addresses
		cbor_helper_head_t *group_addresses_array = cbor_helper_get_array(item, 7);
		if(group_addresses_array == NULL) {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to get group addresses array from element at index %u", i);
			continue;
		}
		if(group_addresses_array->property.size == 0) {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Group addresses array is empty for element at index %u", i);
			continue;
		}
		entry->group_addresses_count = group_addresses_array->property.size;
		entry->group_addresses = KNX_MALLOC(sizeof(uint32_t) * entry->group_addresses_count);
		if(entry->group_addresses == NULL) {
			ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to allocate memory for group addresses for element at index %u", i);
			continue;
		}
		for(int j = 0; j < group_addresses_array->property.size; j++)
		{
			cbor_helper_head_t *group_address_item = cbor_helper_get_element_at(group_addresses_array, j);
			if(group_address_item == NULL) {
				ESP_LOGE(TAG, "resource_fp_g_post_handler: Failed to get group address element at index %u for element at index %u", j, i);
				continue;
			}
			if(group_address_item->property.type != CBOR_TYPE_UINT64) {
				ESP_LOGE(TAG, "resource_fp_g_post_handler: Group address element at index %u for element at index %u is not an unsigned integer", j, i);
				continue;
			}
			if(group_address_item->property.value.u64 == NULL) {
				ESP_LOGE(TAG, "resource_fp_g_post_handler: Group address element at index %u for element at index %u has no value", j, i);
				continue;
			}
			entry->group_addresses[j] = (uint32_t)*group_address_item->property.value.u64;
		}

		group_object_table_add_entry(entry);
	}

	cbor_helper_free(cbor_data);

	group_object_table_save();
	group_object_table_print();

	// unsigned char buf[3];
	// coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	// coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

void resource_fp_r_p_handler(bool is_recipient, cbor_helper_head_t *cbor_data)
{
	ESP_LOGI(TAG, "resource_fp_r_p_handler: Got Array of length: %u", (unsigned)cbor_data->property.size);
	for(int i = 0; i < cbor_data->property.size; i++)
	{
		ESP_LOGI(TAG, "resource_fp_r_p_handler: Processing element at index %u", i);
		cbor_helper_head_t *item = cbor_helper_get_element_at(cbor_data, i);
		if(item == NULL) {
			ESP_LOGE(TAG, "resource_fp_r_p_handler: Failed to get element at index %u", i);
			continue;
		}

		repu_entry_t *entry = KNX_MALLOC(sizeof(repu_entry_t));
		if(entry == NULL) {
			ESP_LOGE(TAG, "resource_fp_r_p_handler: Failed to allocate memory for group object entry");
			continue;
		}

		uint64_t temp_value = 0;
		// Get id
		if(cbor_helper_get_uint64(item, 0, &temp_value) == ESP_OK) {
			if(temp_value > UINT16_MAX) {
				ESP_LOGE(TAG, "resource_fp_r_p_handler: Value for id at index %u is greater than 0xFFFF large: %" PRIx64, i, temp_value);
				continue;
			}
			entry->id = (uint16_t)temp_value;
		}
		else {
			ESP_LOGE(TAG, "resource_fp_r_p_handler: Failed to get id value from element at index %u", i);
			continue;
		}

		// Get group id
		if(cbor_helper_get_uint64(item, 13, &temp_value) == ESP_OK) {
			if(temp_value > 0xFFFFFFFF) {
				ESP_LOGE(TAG, "resource_fp_r_p_handler: Group id is greater than 0xFFFFFFFF at index %u: %" PRIx64, i, temp_value);
				continue;
			}
			entry->group_id = (uint32_t)temp_value;
		}
		else {
			ESP_LOGE(TAG, "resource_fp_r_p_handler: Failed to get group id from element at index %u", i);
			continue;
		}

		// Get group addresses
		cbor_helper_head_t *group_addresses_array = cbor_helper_get_array(item, 7);
		if(group_addresses_array == NULL) {
			ESP_LOGE(TAG, "resource_fp_r_p_handler: Failed to get group addresses array from element at index %u", i);
			continue;
		}
		if(group_addresses_array->property.size == 0) {
			ESP_LOGE(TAG, "resource_fp_r_p_handler: Group addresses array is empty for element at index %u", i);
			continue;
		}
		entry->group_addresses_count = group_addresses_array->property.size;
		entry->group_addresses = KNX_MALLOC(sizeof(uint32_t) * entry->group_addresses_count);
		if(entry->group_addresses == NULL) {
			ESP_LOGE(TAG, "resource_fp_r_p_handler: Failed to allocate memory for group addresses for element at index %u", i);
			continue;
		}
		for(int j = 0; j < group_addresses_array->property.size; j++)
		{
			cbor_helper_head_t *group_address_item = cbor_helper_get_element_at(group_addresses_array, j);
			if(group_address_item == NULL) {
				ESP_LOGE(TAG, "resource_fp_r_p_handler: Failed to get group address element at index %u for element at index %u", j, i);
				continue;
			}
			if(group_address_item->property.type != CBOR_TYPE_UINT64) {
				ESP_LOGE(TAG, "resource_fp_r_p_handler: Group address element at index %u for element at index %u is not an unsigned integer", j, i);
				continue;
			}
			if(group_address_item->property.value.u64 == NULL) {
				ESP_LOGE(TAG, "resource_fp_r_p_handler: Group address element at index %u for element at index %u has no value", j, i);
				continue;
			}
			entry->group_addresses[j] = (uint32_t)*group_address_item->property.value.u64;
		}

		if(is_recipient)
			repu_table_add_recipient(entry);
		else
			repu_table_add_publisher(entry);
	}

	if(is_recipient) {
		repu_table_save_recipient();
		repu_table_print_recipient();
	} else {
		repu_table_save_publisher();
		repu_table_print_publisher();
	}
}

static void resource_fp_r_post_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "POST request received for /fp/r");

	uint8_t buffer_size = 100;
	uint8_t *buffer = KNX_MALLOC(buffer_size);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "resource_fp_r_post_handler: Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "resource_fp_r_post_handler: Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

	ESP_LOGI(TAG, "resource_fp_r_post_handler: Got Array of length: %u", (unsigned)cbor_data->property.size);

	cbor_helper_print(cbor_data, 1);
	resource_fp_r_p_handler(true, cbor_data);
	cbor_helper_free(cbor_data);

	// unsigned char buf[3];
	// coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	// coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

static void resource_fp_p_post_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "POST request received for /fp/p");

	uint8_t buffer_size = 100;
	uint8_t *buffer = KNX_MALLOC(buffer_size);
	if(buffer == NULL) {
		ESP_LOGE(TAG, "resource_fp_p_post_handler: Failed to allocate memory for response buffer");
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_INTERNAL_ERROR);
		return;
	}

    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "resource_fp_p_post_handler: Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

	ESP_LOGI(TAG, "resource_fp_p_post_handler: Got Array of length: %u", (unsigned)cbor_data->property.size);

	cbor_helper_print(cbor_data, 1);

	resource_fp_r_p_handler(false, cbor_data);

	// unsigned char buf[3];
	// coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	// coap_add_data(response, len, buffer);

	KNX_FREE(buffer);
}

void resource_fp_init(coap_context_t *ctx)
{
    coap_resource_t *resource = coap_resource_init(coap_make_str_const("fp/g"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() failed fp/g");
		coap_free_context(ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_POST, resource_fp_g_post_handler);
	coap_add_resource(ctx, resource);

    resource = coap_resource_init(coap_make_str_const("fp/r"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() failed fp/r");
		coap_free_context(ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_POST, resource_fp_r_post_handler);
	coap_add_resource(ctx, resource);
	

    resource = coap_resource_init(coap_make_str_const("fp/p"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() failed fp/p");
		coap_free_context(ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_POST, resource_fp_p_post_handler);
	coap_add_resource(ctx, resource);
}