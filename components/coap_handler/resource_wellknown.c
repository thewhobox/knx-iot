#include "resource_wellknown.h"

#include "knx_device_config.h"
#include "coap_handler.h"
#include "cbor_helper.h"

#include "coap3/coap_session_internal.h"

#include "esp_log.h"

static const char *TAG = "CoAP Handler<.well-known>";

static void resource_wellknown_core_get_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
	ESP_LOGI(TAG, "GET request received for /.well-known/core");
    if(query->length == 0)
    {
        ESP_LOGI(TAG, "No query parameters -> ignoring");
    } else {
        ESP_LOGI(TAG, "Query: %.*s", (int)query->length, query->s);

        if(strncmp((const char *)query->s, "ep=", 3) == 0)
        {
             bool send_response = true;
            uint8_t *val_start = query->s + 3;
            
            uint8_t *serialnumber = knx_device_config_get_serialnumber();
            uint16_t device_individual_address = knx_device_config_get_individual_address();
            uint64_t device_installation_id = knx_device_config_get_installation_id();

            if(strncmp((const char *)val_start, "knx://sn.", 9) == 0)
            {
                val_start += 9;
                uint8_t serial[6];

                for(int i = 0; i < 6; i++)
                {
                    char byte_str[3] = {0};
                    memcpy(byte_str, val_start + i*2, 2);
                    serial[i] = (uint8_t)strtoul(byte_str, NULL, 16);
                }

                if(memcmp(serial, serialnumber, 6) != 0)
                {
                    ESP_LOGI(TAG, "Endpoint query parameter does not match our serial number -> ignoring");
                    ESP_LOGI(TAG, "Got: %.2x%.2x%.2x%.2x%.2x%.2x Expected: %.2x%.2x%.2x%.2x%.2x%.2x", 
                        serial[0], serial[1], serial[2], serial[3], serial[4], serial[5],
                        serialnumber[0], serialnumber[1], serialnumber[2], serialnumber[3], serialnumber[4], serialnumber[5]);
                    return;
                }

                send_response = true;
            } else if (strncmp((const char *)val_start, "knx://ia.", 9) == 0) {
                //val_start += 9;

                const char *ia_pos = strnstr((const char *)val_start, "ia.", query->length - (val_start - query->s));
                if (!ia_pos)
                    return;

                ia_pos += 3; // hinter "ia." → zeigt auf "5e1e4fe567.2003"

                // Ende suchen: nächster Punkt
                const char *end = strchr(ia_pos, '.');
                if (!end)
                    return;

                size_t len = end - ia_pos + 1; // +1 für Nullterminator 
                uint8_t iia_out[len];
                memcpy(iia_out, ia_pos, len);
                iia_out[len] = '\0';

                uint64_t ia_value = strtoull((const char *)iia_out, NULL, 16);
                if(ia_value != device_installation_id)
                {
                    ESP_LOGI(TAG, "Endpoint query parameter does not match our installation ID -> ignoring");
                    ESP_LOGI(TAG, "Got: %" PRIu64 " Expected: %" PRIu64, ia_value, device_installation_id);
                    return;
                }
                
                len = query->length - (len + 9 + 1); // +9 knx://ia. +1 Punkt hinter der Installation ID
                uint8_t ia_out[len];
                memcpy(ia_out, end + 1, len); // +1 um den Punkt hinter der IIA zu überspringen
                ia_out[len] = '\0';

                ESP_LOGI(TAG, "Parsed individual address: %s", ia_out);
                uint16_t individual_address = strtoull((const char *)ia_out, NULL, 16);
                ESP_LOGI(TAG, "Parsed individual address (numeric): %.4X", individual_address);
                if(individual_address != device_individual_address)
                {
                    ESP_LOGI(TAG, "Endpoint query parameter does not match our individual address -> ignoring");
                    ESP_LOGI(TAG, "Got: %04X Expected: %04X", individual_address, device_individual_address);
                    return;
                }

                send_response = true;
            } else {
                ESP_LOGI(TAG, "Endpoint query parameter does not match our endpoint name -> ignoring");
            }

            if(send_response)
            {
                uint8_t payload[100];
                int payload_len = snprintf((char *)payload, sizeof(payload), "<>;ep=\"knx://sn.%.2X%.2X%.2X%.2X%.2X%.2X knx://ia.%" PRIx64 ".%.4x\"",
                    serialnumber[0], serialnumber[1], serialnumber[2], serialnumber[3], serialnumber[4], serialnumber[5],
                    device_installation_id, device_individual_address);

                unsigned char buf[3];
                coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_LINK_FORMAT), buf);
                coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
                //const char *response_payload = "<>;ep=\"knx://sn.00fa10020701 knx://ia.0.ffff\"";
                coap_add_data(response, payload_len, payload);
            }
        } else {
            ESP_LOGI(TAG, "Unknown query parameters -> ignoring");
        }
    }
}

static void resource_wellknown_knx_ia_post_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "POST request received for /.well-known/knx/ia");

    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

    uint64_t temp_value = 0;
    // read individual address (key 12) and installation ID (key 26) from parsed CBOR data
    if(cbor_helper_get_uint64(cbor_data, CBOR_KEY_WKIA_INDIVIDUAL_ADDRESS, &temp_value) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get new individual address from parsed CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        cbor_helper_free(cbor_data);
        return;
    }
    uint16_t new_ia = (uint16_t)(temp_value & 0xFFFF);
    ESP_LOGI(TAG, "Got new IA: %u.%u.%u", (new_ia >> 12) & 0xF, (new_ia >> 8) & 0xF, new_ia & 0xFF);

    if(cbor_helper_get_uint64(cbor_data, CBOR_KEY_WKIA_INSTALLATION_ID, &temp_value) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get installation ID from parsed CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        cbor_helper_free(cbor_data);
        return;
    }
    cbor_helper_free(cbor_data);
    ESP_LOGI(TAG, "Got installation ID: %" PRIu64, temp_value);

    knx_device_config_set_individual_address(new_ia);
    knx_device_config_set_installation_id(temp_value);
    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
}

static void resource_wellknown_knx_post_handler(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
{
    ESP_LOGI(TAG, "POST request received for /.well-known/knx");

    size_t payload_len = 0;
    const uint8_t *payload = NULL;
    coap_get_data(request, &payload_len, &payload);

    uint64_t temp_value = 0;

    size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Free SPIRAM: %u bytes", free_size);
    cbor_helper_head_t *cbor_data = cbor_helper_parse(payload, payload_len);
    if(cbor_data == NULL) {
        ESP_LOGE(TAG, "Failed to parse CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

    // read individual address (key 12) and installation ID (key 26) from parsed CBOR data
    if(cbor_helper_get_uint64(cbor_data, CBOR_KEY_WK_ERASE_CODE, &temp_value) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get erase code from parsed CBOR data");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        cbor_helper_free(cbor_data);
        return;
    }
    ESP_LOGI(TAG, "Got Erase code: %" PRIu64, temp_value);
    cbor_helper_free(cbor_data);
    ESP_LOGI(TAG, "Erased");
    size_t erased = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Free SPIRAM after erase: %u bytes (erased %u bytes)", erased, erased - free_size);

    bool valid_code = !(temp_value != 1 && temp_value != 2 && temp_value != 3 && temp_value != 7);

    cbor_helper_head_t *cbor_head = NULL;
    cbor_data = cbor_helper_add_text_string(&cbor_head, "code");
    cbor_data = cbor_helper_add_uint64(&cbor_data, valid_code ? 0 : 2); // 0 = valid code, 2 = unsupported code
    if(valid_code)
    {
        cbor_data = cbor_helper_add_text_string(&cbor_data, "time");
        cbor_data = cbor_helper_add_uint64(&cbor_data, 3); // 3s to reboot
    }

    uint8_t buffer[CBOR_BUFFER_SIZE];
    size_t len = cbor_helper_encode(buffer, CBOR_BUFFER_SIZE, cbor_head);
        
	unsigned char buf[3];
	coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_CBOR), buf);
	coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
	coap_add_data(response, len, buffer);

    // TODO set timer to reboot if !no_valid_code
}

void resource_wellknown_init(coap_context_t *ctx)
{
	coap_resource_t *resource = coap_resource_init(coap_make_str_const(".well-known/core"), 0);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() .well-known/core failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_wellknown_core_get_handler);
	coap_add_resource(ctx, resource);

	resource = coap_resource_init(coap_make_str_const(".well-known/knx/ia"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() .well-known/knx/ia failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_POST, resource_wellknown_knx_ia_post_handler);
	coap_add_resource(ctx, resource);

	resource = coap_resource_init(coap_make_str_const(".well-known/knx"), COAP_RESOURCE_FLAGS_OSCORE_ONLY);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() .well-known/knx failed");
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_POST, resource_wellknown_knx_post_handler);
	coap_add_resource(ctx, resource);
}