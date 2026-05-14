#include <stdio.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"

#include "driver/gpio.h"
#include <inttypes.h>

#include "knx_iot.h"

static const char *TAG = "KNX-IoT";

#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASSWORD      CONFIG_ESP_WIFI_PASSWORD

static void start_mdns(void)
{
    ESP_LOGI(TAG, "Starting mDNS");

    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("knx-00faf9048288"));
    ESP_ERROR_CHECK(mdns_instance_name_set("KNX Device"));

    ESP_ERROR_CHECK(mdns_service_add("KNX Service", "_knx", "_udp", 5353, NULL, 0));

    ESP_LOGI(TAG, "mDNS service announced: _knx._udp @ knx-00faf9048288.local");
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
                knx_iot_init();
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

    esp_netif_create_default_wifi_sta();

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

}