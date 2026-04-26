#include <stdio.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"

#include "driver/gpio.h"
#include <inttypes.h>

#include "coap_config.h"
#include "coap3/coap.h"

#ifndef CONFIG_COAP_SERVER_SUPPORT
#error COAP_SERVER_SUPPORT needs to be enabled
#endif /* COAP_SERVER_SUPPORT */

static const char *TAG = "KNX-IoT";

#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASSWORD      CONFIG_ESP_WIFI_PASSWORD


uint8_t device_serial[6] = {0x00, 0xFA, 0x10, 0x02, 0x07, 0x01}; // → 00:FA:10:02:70:01
uint16_t device_individual_address = 0xFFFF; // → FFFF (broadcast address)
uint64_t device_installation_id = 0;

static void start_mdns(void)
{
    ESP_LOGI(TAG, "Starting mDNS");

    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("knx-00fa10020701"));
    ESP_ERROR_CHECK(mdns_instance_name_set("KNX Device"));

    ESP_ERROR_CHECK(mdns_service_add("KNX Service", "_knx", "_udp", 5353, NULL, 0));

    ESP_LOGI(TAG, "mDNS service announced: _knx._udp @ knx-00fa10020701.local");
}

#define CONFIG_COAP_LISTEN_PORT 5683
coap_context_t  *coap_ctx = NULL;


static uint8_t oscore_config[] =
    "master_secret,hex,\"3194BB0BCC341407F06F2A4A837EB4E2\"\n"
    //"master_salt,hex,\"00\"\n"
    //"sender_id,hex,\"00\"\n"
    "id_context,hex,\"0d\"\n"
    "recipient_id,hex,\"0c00fa10020701\"\n"
    "replay_window,integer,32\n"
    // "aead_alg,integer,10\n" is default
    // "hkdf_alg,integer,-10\n" is default
    ;


// // https://github.com/obgm/libcoap/tree/main/examples
// // https://github.com/espressif/idf-extra-components/blob/b49dcaceb3e435b49629bb6e9748e2c38d8a26ac/coap/examples/coap_server/main/coap_server_example_main.c

#include <string.h>

static void hnd_espressif_get(coap_resource_t *resource, coap_session_t *session, const coap_pdu_t *request, const coap_string_t *query, coap_pdu_t *response)
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

                if(memcmp(serial, device_serial, 6) != 0)
                {
                    ESP_LOGI(TAG, "Endpoint query parameter does not match our serial number -> ignoring");
                    ESP_LOGI(TAG, "Got: %.2x%.2x%.2x%.2x%.2x%.2x Expected: %.2x%.2x%.2x%.2x%.2x%.2x", 
                        serial[0], serial[1], serial[2], serial[3], serial[4], serial[5],
                        device_serial[0], device_serial[1], device_serial[2], device_serial[3], device_serial[4], device_serial[5]);
                    return;
                }

                uint8_t payload[100];
                int payload_len = snprintf((char *)payload, sizeof(payload), "<>;ep=\"knx://sn.%.2X%.2X%.2X%.2X%.2X%.2X knx://ia.%" PRIx64 ".%.4x\"",
                    device_serial[0], device_serial[1], device_serial[2], device_serial[3], device_serial[4], device_serial[5],
                    device_installation_id, device_individual_address);

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

                uint16_t individual_address = 0xFFFF;
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
            } else {
                ESP_LOGI(TAG, "Endpoint query parameter does not match our endpoint name -> ignoring");
            }
        } else {
            ESP_LOGI(TAG, "Unknown query parameters -> ignoring");
        }
    }
}

void coap_task(void *pvParameters)
{
	uint wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
	while (1) {
		int result = coap_io_process(coap_ctx, wait_ms);
		if (result < 0) {
            ESP_LOGW(TAG, "coap_io_process() failed with error %d", result);
			break;
		} else if (result && (unsigned)result < wait_ms) {
			/* decrement if there is a result wait time returned */
			wait_ms -= result;
		}
		if (result) {
			/* result must have been >= wait_ms, so reset wait_ms */
			wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
		}
	}
	
    coap_free_context(coap_ctx);
    coap_cleanup();

    vTaskDelete(NULL);
}

static void coap_log_handler (coap_log_t level, const char *message)
{
    //ESP_LOG_LEVEL(level, "CoAP", "%s", message);
    uint32_t esp_level = ESP_LOG_INFO;
    const char *cp = strchr(message, '\n');

    while (cp) {
        ESP_LOG_LEVEL(esp_level, "CoAP", "%.*s", (int)(cp - message), message);
        message = cp + 1;
        cp = strchr(message, '\n');
    }
    if (message[0] != '\000') {
        ESP_LOG_LEVEL(esp_level, "CoAP", "%s", message);
    }
}

void coap_init()
{
	ESP_LOGI(TAG, "Initializing CoAP server");
	coap_startup();
	coap_ctx = coap_new_context(NULL);
	if (!coap_ctx) {
		ESP_LOGE(TAG, "coap_new_context() failed");
		coap_free_context(coap_ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
	}

    coap_set_log_handler(coap_log_handler);
    coap_set_log_level(COAP_LOG_OSCORE);

	coap_context_set_block_mode(coap_ctx, COAP_BLOCK_SINGLE_BODY); // COAP_BLOCK_USE_LIBCOAP
	coap_context_set_max_idle_sessions(coap_ctx, 20);
	coap_context_set_keepalive(coap_ctx, 30);

	uint32_t scheme_hint_bits = coap_get_available_scheme_hint_bits(1, 0, COAP_PROTO_UDP);
	ESP_LOGI(TAG, "Available scheme hint bits: 0x%02x", scheme_hint_bits);
	coap_addr_info_t *info_list = coap_resolve_address_info(coap_make_str_const("::"), CONFIG_COAP_LISTEN_PORT, 0,
                                              0, 0,
                                              0,
                                              scheme_hint_bits,
                                              COAP_RESOLVE_TYPE_LOCAL);
	if (info_list == NULL) {
        ESP_LOGE(TAG, "coap_resolve_address_info() failed");
		coap_free_context(coap_ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}

	bool have_ep = 0;
	coap_addr_info_t *info;
	ESP_LOGI(TAG, "Creating endpoints...");
	for (info = info_list; info != NULL; info = info->next) {
		coap_endpoint_t *ep;

		//ESP_LOGI(TAG, "Creating endpoint for proto %u for address %i and port %i", info->proto, info->addr.addr.sin6.sin6_addr., info->addr.port);
		ep = coap_new_endpoint(coap_ctx, &info->addr, info->proto);
		if (!ep) {
			ESP_LOGW(TAG, "cannot create endpoint for proto %u", info->proto);
		} else {
			have_ep = 1;
		}
	}
	coap_free_address_info(info_list);
	if (!have_ep) {
		ESP_LOGE(TAG, "No endpoints available");
		coap_free_context(coap_ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}

    size_t free_mem = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Free internal heap memory: %u bytes", free_mem);

	coap_str_const_t config = { sizeof(oscore_config) - 1, oscore_config };
	coap_oscore_conf_t *oscore_conf;
	oscore_conf = coap_new_oscore_conf(config, NULL, NULL, 160);

    // coap_delete_bin_const(oscore_conf->sender_id);
	// oscore_conf->sender_id = coap_new_bin_const(NULL, 0); // empty sender ID
    
    coap_context_oscore_server(coap_ctx, oscore_conf);

    // coap_oscore_conf_t *oscore_conf = heap_caps_malloc(sizeof(coap_oscore_conf_t), MALLOC_CAP_SPIRAM);
    
    free_mem = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Free internal heap memory after allocating OSCORE config: %u bytes", free_mem);

	coap_resource_t *resource = coap_resource_init(coap_make_str_const(".well-known/core"), 0);
	if (!resource) {
		ESP_LOGE(TAG, "coap_resource_init() failed");
		coap_free_context(coap_ctx);
		coap_cleanup();
    	vTaskDelete(NULL);
		return;
	}
	coap_register_handler(resource, COAP_REQUEST_GET, hnd_espressif_get);
	/* We possibly want to Observe the GETs */
    // coap_resource_set_get_observable(resource, 1);
	coap_add_resource(coap_ctx, resource);

    ESP_LOGI(TAG, "CoAP server initialized, listening on port %d", CONFIG_COAP_LISTEN_PORT);
    esp_netif_t *netif = NULL;
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        char buf[8];
        netif = esp_netif_next_unsafe(netif);
        esp_netif_get_netif_impl_name(netif, buf);
        /* When adding IPV6 esp-idf requires ifname param to be filled in */
        int x = coap_join_mcast_group_intf(coap_ctx, "ff02::fd", buf);
        ESP_LOGI(TAG, "Joined multicast group ff02::fd on interface %s (%i)", buf, x);
    }

	xTaskCreate(coap_task, "coap", 8 * 1024, NULL, 5, NULL);
}


bool already_got_ipv6 = false;


static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected");
            
            esp_netif_t* sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (sta) {
                esp_err_t err = esp_netif_create_ip6_linklocal(sta);
                ESP_LOGI(TAG, "IPv6 LL create: %s", esp_err_to_name(err));
            }
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
            esp_wifi_connect();
            break;
        default:
            ESP_LOGI(TAG, "WiFi event: %ld", event_id);
            break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "STA IPv4: " IPSTR, IP2STR(&event->ip_info.ip));
        } else if (event_id == IP_EVENT_GOT_IP6) {
            ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
            ESP_LOGI(TAG, "STA IPv6: " IPV6STR, IPV62STR(event->ip6_info.ip));
            if(!already_got_ipv6)
            {
                already_got_ipv6 = true;
                start_mdns();
                coap_init();
            }
        }
    }
}


void app_main(void)
{
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("phy", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("phy_init", ESP_LOG_NONE);
    esp_log_level_set("net80211", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_handlers", ESP_LOG_NONE);
    esp_log_level_set("mdns_mem", ESP_LOG_NONE);
    

    // LED
    // https://components.espressif.com/components/espressif/led_strip/versions/3.0.3/readme


    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    // esp_netif_create_default_wifi_sta();

    //esp_netif_join_ip6_multicast_group(sta_netif, )


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ESP_WIFI_SSID);
    ESP_LOGI(TAG, "WiFi password: %s", ESP_WIFI_PASSWORD);

    wifi_config_t wifi_config = { 0 };
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", ESP_WIFI_SSID);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", ESP_WIFI_PASSWORD);

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "LWIP IPV6 MLD is %i", LWIP_IPV6_MLD);
}