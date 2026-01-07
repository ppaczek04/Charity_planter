#include "wifi_manager.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "mqtt_manager.h"

#include <time.h>
#include "esp_log.h"
#include "esp_sntp.h"

#define TAG "WIFI_MGR"
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t s_wifi_event_group;



static void init_time(void) {
    ESP_LOGI("TIME", "Inicjalizacja SNTP...");

    // Strefa czasowa: Polska
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    // Konfiguracja SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.nist.gov");
    esp_sntp_init();

    time_t now = 0;
    int retry = 0;
    const int retry_count = 10;

    while (now < 1700000000 && ++retry < retry_count) {
        ESP_LOGI("TIME", "Czekam na synchronizację czasu... (%d)", retry);
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
    }

    if (now < 1700000000) {
        ESP_LOGE("TIME", "❌ Synchronizacja NTP NIE POWIODŁA SIĘ");
    } else {
        ESP_LOGI("TIME", "✅ Czas zsynchronizowany: %ld", now);
    }
}

// Obsługa zdarzeń Wi-Fi i IP
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGW(TAG, "Rozłączono z WiFi. Próba ponownego połączenia...");
        mqtt_stop_activity();
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        init_time();
        ESP_LOGI(TAG, "Uzyskano IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        mqtt_start_activity();
    }
}


void wifi_manager_init(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Pobieranie konfiguracji z NVS
    char ssid[32] = {0};
    char password[64] = {0};

    if (get_wifi_ssid(ssid, sizeof(ssid)) != ESP_OK || 
        get_wifi_password(password, sizeof(password)) != ESP_OK) {
        ESP_LOGE(TAG, "Błąd: Brak SSID lub Hasła w NVS! Uruchom tryb konfiguracji.");
        return; 
    }

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // Bezpieczne kopiowanie stringów
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Inicjalizacja WiFi zakończona. SSID: %s", ssid);
}
