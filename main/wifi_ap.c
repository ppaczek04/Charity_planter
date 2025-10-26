#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define EXAMPLE_ESP_WIFI_SSID_AP      "ESP32_AP"
#define EXAMPLE_ESP_WIFI_PASS_AP      "12345678"
#define EXAMPLE_MAX_STA_CONN          4

static const char *TAG_AP = "wifi_ap";

void wifi_init_ap(void)
{
    // Inicjalizacja NVS i netif
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Tworzymy interfejs AP
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, EXAMPLE_ESP_WIFI_SSID_AP, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID_AP);
    strncpy((char *)wifi_config.ap.password, EXAMPLE_ESP_WIFI_PASS_AP, sizeof(wifi_config.ap.password));
    wifi_config.ap.max_connection = EXAMPLE_MAX_STA_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    if (strlen(EXAMPLE_ESP_WIFI_PASS_AP) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_AP, "AP mode started. SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID_AP, EXAMPLE_ESP_WIFI_PASS_AP);
}

