#include "mqtt_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include "cJSON.h"
#include "ble_config.h"
#include "globals.h"
#include "esp_err.h"

#define TAG "MQTT_MGR"

static esp_mqtt_client_handle_t client = NULL;
static bool is_connected = false;
static bool s_mqtt_started = false;

static char broker_url[64];
static char username[32];
static char password[64];

static water_command_callback_t water_callback = NULL;

static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Połączono z MQTT");
        is_connected = true;
        
        int msg_id = esp_mqtt_client_subscribe(client, WATER_TOPIC, 1);
        ESP_LOGI(TAG, "Zasubskrybowano %s, msg_id=%d", WATER_TOPIC, msg_id);
        
        resend_messages_from_nvs(client);
        
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Rozłączono z MQTT");
        is_connected = false;
        
    } else if (event_id == MQTT_EVENT_DATA) {
        ESP_LOGI(TAG, "Otrzymano wiadomość: %.*s na topiku %.*s", 
                 event->data_len, event->data, event->topic_len, event->topic);
        
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
    s_mqtt_started = true; // Oznaczamy flagę, że klient ruszył
}

static void process_sensor_data(const char *topic, double value, const char *unit, enum Parameter param_type) {
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "value", value);
    if (unit != NULL) {
        cJSON_AddStringToObject(root, "unit", unit);
    }
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == NULL) {
        ESP_LOGE(TAG, "Błąd alokacji JSON");
        return;
    }

    if (is_connected && client != NULL) {
        int msg_id = esp_mqtt_client_publish(client, topic, json_str, 0, 1, 0);
        if (msg_id != -1) {
            ESP_LOGI(TAG, "Wysłano na %s: %s", topic, json_str);
        } else {
            ESP_LOGE(TAG, "Błąd wysyłania (mimo flagi connected). Zapisuję do NVS.");
            save_message_to_nvs(json_str, param_type);
        }
    } else {
        ESP_LOGW(TAG, "Brak połączenia MQTT. Zapisuję do NVS: %s", topic);
        save_message_to_nvs(json_str, param_type);
    }

    free(json_str);
}

void mqtt_send_sensor_data(int soil, float temp, float press) {
    process_sensor_data(TOPIC_SOIL, (double)soil, "%", SOIL_HUMIDITY);
    process_sensor_data(TOPIC_TEMP, (double)temp, "C", TEMPERATURE);
    process_sensor_data(TOPIC_PRESS, (double)press, "hPa", PRESSURE);
}

void mqtt_send_coin_event(void) {
    process_sensor_data(TOPIC_COIN, 1.0, "PLN", COIN_INSERTED);
}

void set_water_command_callback(water_command_callback_t callback) {
    water_callback = callback;
}

void mqtt_stop_activity(void) {
    if (client != NULL && s_mqtt_started) {
        esp_mqtt_client_stop(client);
        s_mqtt_started = false;
    }
}

void mqtt_start_activity(void) {
    if (client != NULL && !s_mqtt_started) {
        esp_mqtt_client_start(client);
        s_mqtt_started = true;
    }
}