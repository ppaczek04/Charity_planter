#include "mqtt_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include "cJSON.h"
#include "ble_config.h"

#define TAG "MQTT_MGR"

#define TOPIC_PREFIX "user1/esp32_test"

static esp_mqtt_client_handle_t client = NULL;
static bool is_connected = false;

static char broker_url[64];
static char username[32];
static char password[64];

static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Połączono z MQTT");
        is_connected = true;
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Rozłączono z MQTT");
        is_connected = false;
    }
}

void mqtt_manager_init(void) {
    if (get_broker_url(broker_url, sizeof(broker_url)) != ESP_OK) {
        ESP_LOGE(TAG, "Brak URL brokera w NVS");
        return;
    }
    get_broker_username(username, sizeof(username));
    get_broker_password(password, sizeof(password));

    ESP_LOGW(TAG, "Próba połączenia z: %s", broker_url);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_url,
    };
    
    if (username[0] != 0) mqtt_cfg.credentials.username = username;
    if (password[0] != 0) mqtt_cfg.credentials.authentication.password = password;

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void publish_single_value(const char *sensor_type, double value, const char *unit) {
    if (!is_connected || !client) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/%s", TOPIC_PREFIX, sensor_type);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "value", value);
    
    if (unit != NULL) {
        cJSON_AddStringToObject(root, "unit", unit);
    }

    char *json_str = cJSON_PrintUnformatted(root);

    int msg_id = esp_mqtt_client_publish(client, topic, json_str, 0, 1, 0);
    
    if (msg_id != -1) {
        ESP_LOGI(TAG, "Wysłano na %s: %s", topic, json_str);
    } else {
        ESP_LOGE(TAG, "Błąd wysyłania na %s", topic);
    }

    free(json_str);
    cJSON_Delete(root);
}

void mqtt_send_sensor_data(int soil, float temp, float press) {
    if (!is_connected) {
        ESP_LOGW(TAG, "MQTT niepołączone, pomijam wysyłanie danych.");
        return;
    }

    ESP_LOGI(TAG, "Wysyłanie danych z czujników...");

    publish_single_value("soil", (double)soil, "%");
    publish_single_value("temperature", (double)temp, "C");
    publish_single_value("pressure", (double)press, "hPa");
}

void mqtt_send_coin_event(void) {
    publish_single_value("coin_inserted", 1.0, "PLN");
}