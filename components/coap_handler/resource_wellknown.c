#include "resource_wellknown.h"

#include "knx_device_config.h"

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
            uint8_t *val_start = query->s + 3;
            
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

                uint8_t *serialnumber = knx_device_config_get_serialnumber();
                if(memcmp(serial, serialnumber, 6) != 0)
                {
                    ESP_LOGI(TAG, "Endpoint query parameter does not match our serial number -> ignoring");
                    ESP_LOGI(TAG, "Got: %.2x%.2x%.2x%.2x%.2x%.2x Expected: %.2x%.2x%.2x%.2x%.2x%.2x", 
                        serial[0], serial[1], serial[2], serial[3], serial[4], serial[5],
                        serialnumber[0], serialnumber[1], serialnumber[2], serialnumber[3], serialnumber[4], serialnumber[5]);
                    return;
                }

                uint8_t payload[100];
                int payload_len = snprintf((char *)payload, sizeof(payload), "<>;ep=\"knx://sn.%.2X%.2X%.2X%.2X%.2X%.2X knx://ia.%" PRIx64 ".%.4x\"",
                    serialnumber[0], serialnumber[1], serialnumber[2], serialnumber[3], serialnumber[4], serialnumber[5],
                    device_installation_id, device_individual_address);

                unsigned char buf[3];
                coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_LINK_FORMAT), buf);
                coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
                //const char *response_payload = "<>;ep=\"knx://sn.00fa10020701 knx://ia.0.ffff\"";
                coap_add_data(response, payload_len, payload);
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
                uint8_t out[len];
                memcpy(out, ia_pos, len);
                out[len] = '\0';

                uint64_t ia_value = strtoull((const char *)out, NULL, 10);
                if(ia_value != device_installation_id)
                {
                    ESP_LOGI(TAG, "Endpoint query parameter does not match our individual address -> ignoring");
                    ESP_LOGI(TAG, "Got: %" PRIu64 " Expected: %" PRIu64, ia_value, device_installation_id);
                    return;
                }

                uint16_t individual_address = knx_device_config_get_individual_address();
                for(int i = 0; i < 6; i++)
                {
                    uint8_t *val = (uint8_t *)&individual_address;
                    char byte_str[3] = {0};
                    memcpy(byte_str, val_start + i*2, 2);
                    val[i] = (uint8_t)strtoul(byte_str, NULL, 16);
                }
                if(individual_address != device_individual_address)
                {
                    ESP_LOGI(TAG, "Endpoint query parameter does not match our individual address -> ignoring");
                    ESP_LOGI(TAG, "Got: %04X Expected: %04X", individual_address, device_individual_address);
                    return;
                }

                ESP_LOGW(TAG, "Received valid query parameters but not implemented wp ia -> ignoring");
            } else {
                ESP_LOGI(TAG, "Endpoint query parameter does not match our endpoint name -> ignoring");
            }
        } else {
            ESP_LOGI(TAG, "Unknown query parameters -> ignoring");
        }
    }
}

void resource_wellknown_init(coap_context_t *ctx)
{
	coap_resource_t *resource = coap_resource_init(coap_make_str_const(".well-known/core"), 0);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() failed");
		coap_free_context(ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, resource_wellknown_core_get_handler);
	/* We possibly want to Observe the GETs */
    // coap_resource_set_get_observable(resource, 1);
	coap_add_resource(ctx, resource);
}