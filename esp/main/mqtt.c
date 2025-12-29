#include "mqtt_client.h"
#include "esp_log.h"
#include "globals.h"
#include "pump_manager.h" // Żeby móc włączyć pompkę
#include <string.h>

static esp_mqtt_client_handle_t client = NULL;

/* Funkcja wywoływana, gdy przyjdzie wiadomość z serwera */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT Connected");
            
            // 1. Tworzymy temat do nasłuchiwania: user/device/water
            char sub_topic[100];
            snprintf(sub_topic, sizeof(sub_topic), WATER_CMD_TOPIC, user_mac, device_mac);
            
            // 2. Subskrybujemy (zapisujemy się na powiadomienia)
            esp_mqtt_client_subscribe(client, sub_topic, 0);
            ESP_LOGI(MQTT_TAG, "Subskrybuje temat: %s", sub_topic);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTT_TAG, "Otrzymano wiadomosc!");
            // Sprawdźmy, czy to rozkaz podlewania
            // (W prostym przypadku włączamy pompkę na 3 sekundy na każdy sygnał)
            
            ESP_LOGI(MQTT_TAG, "Rozkaz z aplikacji: PODLEWANIE");
            pump_turn_on();
            // Uwaga: w prawdziwym kodzie async nie używamy delay w callbacku, 
            // ale na początek dla uproszczenia może być:
            // vTaskDelay(pdMS_TO_TICKS(3000)); 
            // pump_turn_off();
            // Lepszym rozwiązaniem jest ustawienie flagi dla głównej pętli.
            break;
            
        default:
            break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://10.101.48.49:1883", // ZMIEŃ IP NA SWOJE!
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// Funkcja pomocnicza do wysyłania danych z innych plików
void mqtt_send_sensor_data(const char *topic_fmt, float value) {
    if (!client) return;
    char topic[100];
    char payload[20];
    
    snprintf(topic, sizeof(topic), topic_fmt, user_mac, device_mac);
    snprintf(payload, sizeof(payload), "%.2f", value);
    
    esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
}