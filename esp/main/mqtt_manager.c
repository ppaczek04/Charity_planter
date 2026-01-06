#include "mqtt_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include "cJSON.h"
#include "ble_config.h"

#define TAG "MQTT_MGR"

#define TOPIC_PREFIX "user1/esp32_test"
#define WATER_TOPIC "user1/esp32_test/water"

static esp_mqtt_client_handle_t client = NULL;
static bool is_connected = false;

static char broker_url[64];
static char username[32];
static char password[64];

static water_command_callback_t water_callback = NULL;

static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Połączono z MQTT");
        is_connected = true;
        
        // Subskrybuj topik z komendami podlewania
        int msg_id = esp_mqtt_client_subscribe(client, WATER_TOPIC, 1);
        ESP_LOGI(TAG, "Subskrybowano %s, msg_id=%d", WATER_TOPIC, msg_id);
        
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Rozłączono z MQTT");
        is_connected = false;
        
    } else if (event_id == MQTT_EVENT_DATA) {
        ESP_LOGI(TAG, "Otrzymano wiadomość: %.*s na topiku %.*s", 
                 event->data_len, event->data, event->topic_len, event->topic);
        
        // Parsuj JSON
        cJSON *json = cJSON_ParseWithLength(event->data, event->data_len);
        if (json) {
            cJSON *command = cJSON_GetObjectItem(json, "command");
            cJSON *duration = cJSON_GetObjectItem(json, "duration_sec");
            
            if (command && cJSON_IsString(command) && 
                strcmp(command->valuestring, "WATER_ON") == 0) {
                
                float duration_sec = 0.5; // domyślnie
                if (duration && cJSON_IsNumber(duration)) {
                    duration_sec = (float)duration->valuedouble;
                }
                
                ESP_LOGI(TAG, "Komenda WATER_ON, czas: %.1fs", duration_sec);
                
                if (water_callback) {
                    water_callback(duration_sec);
                }
            }
            
            cJSON_Delete(json);
        }
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

void set_water_command_callback(water_command_callback_t callback) {
    water_callback = callback;
}