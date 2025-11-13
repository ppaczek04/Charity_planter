#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "mqtt_client.h"   // biblioteka klienta MQTT
#include <stdbool.h>

// -------------------------------------------------------------
// 🔹 Ustawienia sieci Wi-Fi i MQTT
// -------------------------------------------------------------
#define EXAMPLE_ESP_WIFI_SSID  "realme8"    // nazwa hotspotu (SSID)
#define EXAMPLE_ESP_WIFI_PASS  "12345678"      // hasło do Wi-Fi
#define LED_PIN                2               // pin LED (GPIO2 na ESP32)
#define LAPTOP_IP              "10.179.173.7" // adres IP laptopa z brokerem MQTT (np. Docker z Mosquitto)

// -------------------------------------------------------------
// 🔹 Definicje flag i obiektów FreeRTOS do obsługi połączeń
// -------------------------------------------------------------
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// -------------------------------------------------------------
// 🔹 Zmienne globalne do stanu Wi-Fi
// -------------------------------------------------------------
static const char *TAG = "wifi_station";  // znacznik do logów ESP-IDF
bool wifi_connected = false;              // czy Wi-Fi jest połączone?
static bool wifi_connecting = true;       // czy trwa próba połączenia?

// -------------------------------------------------------------
// 🔹 Zadanie migania LED — pokazuje stan połączenia Wi-Fi
// -------------------------------------------------------------
// LED miga szybko, gdy łączenie trwa,
// świeci ciągle po połączeniu,
// gaśnie, gdy brak sieci.
static void blink_led_task(void *pvParameter)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while (1) {
        if (wifi_connecting) {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(200 / portTICK_PERIOD_MS);
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        } else if (wifi_connected) {
            gpio_set_level(LED_PIN, 1);
        } else {
            gpio_set_level(LED_PIN, 0);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// -------------------------------------------------------------
// 🔹 Obsługa zdarzeń MQTT — reaguje na połączenie i rozłączenie
// -------------------------------------------------------------
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("MQTT", "✅ Połączono z brokerem MQTT");
            // wysyłamy przykładową wiadomość po udanym połączeniu
            esp_mqtt_client_publish(client, "esp32/test", "Hello from ESP32!", 0, 1, 0);
            ESP_LOGI("MQTT", "📤 Wiadomość wysłana na temat esp32/test");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW("MQTT", "⚠️ Rozłączono z brokerem MQTT");
            break;

        default:
            break;
    }
    return ESP_OK;
}

// Wrapper dla handlera — wymagany przez ESP-IDF
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb(event_data);
}

// -------------------------------------------------------------
// 🔹 Uruchomienie klienta MQTT po uzyskaniu połączenia z Wi-Fi
// -------------------------------------------------------------
static void mqtt_app_start(void)
{
    // Budujemy poprawny URI z IP laptopa i portem 1883
    char uri[64];
    snprintf(uri, sizeof(uri), "mqtt://%s:1883", LAPTOP_IP);

    // Konfiguracja klienta MQTT
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = uri
    };

    // Tworzymy i uruchamiamy klienta
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    ESP_LOGI("MQTT", "🚀 Klient MQTT uruchomiony, broker: %s", uri);
}

// -------------------------------------------------------------
// 🔹 Obsługa zdarzeń Wi-Fi — logika połączenia z siecią
// -------------------------------------------------------------
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_connecting = true;
        esp_wifi_connect();  // start próby połączenia
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        wifi_connecting = true;
        ESP_LOGW(TAG, "⚠️ WiFi rozłączone, ponawiam próbę...");
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
        wifi_connecting = false;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✅ Uzyskano IP: " IPSTR, IP2STR(&event->ip_info.ip));

        // po uzyskaniu IP startujemy MQTT
        mqtt_app_start();
    }
}

// -------------------------------------------------------------
// 🔹 Inicjalizacja Wi-Fi w trybie stacji (łączenie do routera/AP)
// -------------------------------------------------------------
void wifi_init_sta(void)
{
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

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));           // tryb stacji
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "🚀 WiFi zainicjalizowane, łączenie z %s", EXAMPLE_ESP_WIFI_SSID);
}

// -------------------------------------------------------------
// 🔹 Funkcja główna programu — punkt startowy
// -------------------------------------------------------------
void app_main(void)
{
    // Inicjalizacja pamięci NVS (wymagana przez Wi-Fi i MQTT)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "📡 Uruchamianie trybu STA (Wi-Fi client)");

    // Zadanie migania LED
    xTaskCreate(blink_led_task, "blink_led_task", 2048, NULL, 5, NULL);
    // Uruchomienie Wi-Fi
    wifi_init_sta();
}
