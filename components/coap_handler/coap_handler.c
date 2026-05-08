#include "coap_handler.h"

#include "resource_wellknown.h"
#include "resource_action.h"
#include "resource_dev.h"
#include "resource_fp.h"

#include "esp_log.h"
#include <string.h>

#define CONFIG_COAP_LISTEN_PORT 5683
coap_context_t  *coap_ctx = NULL;

static const char *TAG = "CoAP Handler";

static uint8_t oscore_config[] =
    "master_secret,hex,\"3194BB0BCC341407F06F2A4A837EB4E2\"\n"
    //"master_salt,hex,\"00\"\n"
    "sender_id,hex,\"\"\n"
    "id_context,hex,\"0d\"\n"
    "recipient_id,hex,\"0c00faf9048288\"\n"
    "replay_window,integer,32\n"
    // "aead_alg,integer,10\n" is default
    // "hkdf_alg,integer,-10\n" is default
    ;



// // https://github.com/obgm/libcoap/tree/main/examples
// // https://github.com/espressif/idf-extra-components/blob/b49dcaceb3e435b49629bb6e9748e2c38d8a26ac/coap/examples/coap_server/main/coap_server_example_main.c


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
    coap_context_oscore_server(coap_ctx, oscore_conf);
    free_mem = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Free internal heap memory after allocating OSCORE config: %u bytes", free_mem);

    resource_wellknown_init(coap_ctx);
	resource_action_init(coap_ctx);
	resource_dev_init(coap_ctx);
	resource_fp_init(coap_ctx);

	// TODO make this work?
    // ESP_LOGI(TAG, "CoAP server initialized, listening on port %d", CONFIG_COAP_LISTEN_PORT);
    // esp_netif_t *netif = NULL;
    // for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
    //     char buf[8];
    //     netif = esp_netif_next_unsafe(netif);
    //     esp_netif_get_netif_impl_name(netif, buf);
    //     /* When adding IPV6 esp-idf requires ifname param to be filled in */
    //     int x = coap_join_mcast_group_intf(coap_ctx, "ff02::fd", buf);
    //     ESP_LOGI(TAG, "Joined multicast group ff02::fd on interface %s (%i)", buf, x);
    // }
	// 3/10/5 2.6.4.2 CoAP multicast scopes
	// Link-local: Typically used to query in a single Wi-Fi or Ethernet network segment. 
	int x = coap_join_mcast_group_intf(coap_ctx, "ff02::fd", "st1");
	ESP_LOGI(TAG, "Joined multicast group ff02::fd on interface st1 (%i)", x);
	// Realm-local: Typically used to query in a single mesh topology IPv6 network.
	x = coap_join_mcast_group_intf(coap_ctx, "ff03::fd", "st1");
	ESP_LOGI(TAG, "Joined multicast group ff03::fd on interface st1 (%i)", x);
	// Site-local: Site-local is the default in an installation and spans across networks and stubs (Wi-Fi, Ethernet and Thread network segments). 
	x = coap_join_mcast_group_intf(coap_ctx, "ff05::fd", "st1");
	ESP_LOGI(TAG, "Joined multicast group ff05::fd on interface st1 (%i)", x);

	xTaskCreate(coap_task, "coap", 8 * 1024, NULL, 5, NULL);
}
