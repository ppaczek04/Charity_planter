#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_adc/adc_oneshot.h"

/* --- KONFIGURACJA --- */
#define WIFI_SSID        "A56Pio"
#define WIFI_PASS        "siema123"
#define MQTT_BROKER_URI  "mqtt://10.101.112.49:1883"   // IP twojego laptopa z Mosquitto
#define TAG              "MQTT_SOIL"

#define CZUJNIK_WILGOTNOSCI_CHANNEL ADC_CHANNEL_0  // GPIO36 (VP)

/* --- GLOBALNE --- */
esp_mqtt_client_handle_t mqtt_client;

/* --- Wi-Fi --- */
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "🔌 Łączenie z WiFi...");
    esp_wifi_connect();
}

/* --- MQTT --- */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ Połączono z brokerem MQTT");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "⚠️ Rozłączono z brokerem MQTT");
            break;
        default:
            break;
    }
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/* --- Zadanie odczytu wilgotności gleby --- */
static void soil_moisture_task(void *pvParameter)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,          // Zakres 0–3.3V
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12-bit
    };
    adc_oneshot_config_channel(adc_handle, CZUJNIK_WILGOTNOSCI_CHANNEL, &config);

    int raw_value;
    char payload[64];

    ESP_LOGI(TAG, "🌱 Start pomiarów wilgotności gleby (GPIO36)");

    while (1) {
        adc_oneshot_read(adc_handle, CZUJNIK_WILGOTNOSCI_CHANNEL, &raw_value);

        // (opcjonalnie) konwersja na procent – zależy od czujnika
        int wilgotnosc_proc = 100 - (raw_value * 100 / 4095);

        sprintf(payload, "{\"soil_raw\": %d, \"soil_percent\": %d}", raw_value, wilgotnosc_proc);

        if (mqtt_client) {
            esp_mqtt_client_publish(mqtt_client, "sensors/esp32_1/soil", payload, 0, 1, 0);
            ESP_LOGI(TAG, "📤 Wysłano: %s", payload);
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // co 5 sekund
    }
}

/* --- main() --- */
void app_main(void)
{
    // Inicjalizacja pamięci NVS (konieczna dla WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    mqtt_init();

    // Uruchom zadanie odczytu i publikacji wilgotności
    xTaskCreate(soil_moisture_task, "soil_moisture_task", 4096, NULL, 5, NULL);
}
