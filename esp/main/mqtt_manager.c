#include "mqtt_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include "cJSON.h"
#include "ble_config.h"
#include "globals.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "nvs.h"

#include <time.h>
#include <sys/time.h>

#define TAG "MQTT_MGR"

static esp_mqtt_client_handle_t client = NULL;
static bool is_connected = false;
static bool s_mqtt_started = false;

static char broker_url[64];
static char username[32];
static char password[64];

static char topic_water[128];
static char topic_soil[128];
static char topic_temp[128];
static char topic_press[128];
static char topic_coin[128];
static char topic_config[128];
static char topic_water_ack[128];
static char topic_config_ack[128];

static water_command_callback_t water_callback = NULL;

extern esp_err_t get_owner_id(char *owner_id, size_t size);

static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Połączono z MQTT");
        is_connected = true;

        esp_mqtt_client_subscribe(client, topic_config, 1);
        int msg_id = esp_mqtt_client_subscribe(client, topic_water, 1);
        ESP_LOGI(TAG, "Zasubskrybowano %s, msg_id=%d", topic_water, msg_id);
        
        ESP_LOGI(TAG, "Wysyłanie danych z bufora NVS...");
        resend_messages(NVS_NS_SOIL, topic_soil, client);
        resend_messages(NVS_NS_TEMP, topic_temp, client);
        resend_messages(NVS_NS_PRESS, topic_press, client);
        resend_messages(NVS_NS_COIN, topic_coin, client);
        
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Rozłączono z MQTT");
        is_connected = false;
        
    } else if (event_id == MQTT_EVENT_DATA) {
        ESP_LOGI(TAG, "Otrzymano wiadomość: %.*s na topiku %.*s", 
                 event->data_len, event->data, event->topic_len, event->topic);
 
        if (strncmp(event->topic, topic_config, event->topic_len) == 0) {
            cJSON *json = cJSON_ParseWithLength(event->data, event->data_len);
            if (json) {
                cJSON *intervalItem = cJSON_GetObjectItem(json, "interval");
                if (cJSON_IsNumber(intervalItem)) {
                    int new_seconds = intervalItem->valueint;
                    if (new_seconds >= 5) { 
                        uint32_t new_ms = new_seconds * 1000;
                        save_measurement_interval(new_ms); 
                        ESP_LOGI(TAG, "Zaktualizowano interwał: %d s", new_seconds);
                        
                        char *ack_payload = "{\"status\":\"OK\"}";
                        esp_mqtt_client_publish(client, topic_config_ack, ack_payload, 0, 0, 0);
                        ESP_LOGI(TAG, "Wysłano ACK na: %s", topic_config_ack);
                    }
                }
                cJSON_Delete(json);
            }
        }
        else {
            cJSON *json = cJSON_ParseWithLength(event->data, event->data_len);
            if (json) {
                cJSON *command = cJSON_GetObjectItem(json, "command");
                cJSON *duration = cJSON_GetObjectItem(json, "duration_sec");
                
                if (command && cJSON_IsString(command) && 
                    strcmp(command->valuestring, "WATER_ON") == 0) {
                    
                    float duration_sec = 0.8;
                    if (duration && cJSON_IsNumber(duration)) {
                        duration_sec = (float)duration->valuedouble;
                    }
                    
                    ESP_LOGI(TAG, "Komenda WATER_ON, czas: %.1fs", duration_sec);
                    
                    if (water_callback) {
                        water_callback(duration_sec);
                    }

                    char *ack_payload = "{\"status\":\"OK\"}";
                    esp_mqtt_client_publish(client, topic_water_ack, ack_payload, 0, 0, 0);
                    ESP_LOGI(TAG, "Wysłano ACK na: %s", topic_water_ack);
                }
                cJSON_Delete(json);
            }
        }
    }
}

long get_timestamp_seconds(void) {
    time_t now;
    time(&now);
    return (long) now;
}

static void build_topics(void) {
    char owner_id[64] = {0};
    uint8_t mac_raw[6];
    char mac_str[18] = {0};

    if (get_owner_id(owner_id, sizeof(owner_id)) != ESP_OK || strlen(owner_id) == 0) {
        ESP_LOGW(TAG, "Brak OwnerID! Używam domyślnego 'unknown_user'");
        strcpy(owner_id, "unknown_user");
    }

    esp_read_mac(mac_raw, ESP_MAC_WIFI_STA);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_raw[0], mac_raw[1], mac_raw[2], mac_raw[3], mac_raw[4], mac_raw[5]);

    snprintf(topic_water, sizeof(topic_water), "%s/%s/%s", owner_id, mac_str, TOPIC_SUFFIX_WATER);
    snprintf(topic_soil,  sizeof(topic_soil),  "%s/%s/%s", owner_id, mac_str, TOPIC_SUFFIX_SOIL);
    snprintf(topic_temp,  sizeof(topic_temp),  "%s/%s/%s", owner_id, mac_str, TOPIC_SUFFIX_TEMP);
    snprintf(topic_press, sizeof(topic_press), "%s/%s/%s", owner_id, mac_str, TOPIC_SUFFIX_PRESS);
    snprintf(topic_coin,  sizeof(topic_coin),  "%s/%s/%s", owner_id, mac_str, TOPIC_SUFFIX_COIN);
    snprintf(topic_config, sizeof(topic_config), "%s/%s/%s", owner_id, mac_str, TOPIC_SUFFIX_CONFIG);
    snprintf(topic_water_ack, sizeof(topic_water_ack), "%s/%s/water/ack", owner_id, mac_str);
    snprintf(topic_config_ack, sizeof(topic_config_ack), "%s/%s/config/ack", owner_id, mac_str);

    ESP_LOGI(TAG, "Zbudowano temat gleby: %s", topic_soil);
}

void mqtt_manager_init(void) {
    if (get_broker_url(broker_url, sizeof(broker_url)) != ESP_OK) {
        ESP_LOGE(TAG, "Brak URL brokera w NVS");
        return;
    }
    get_broker_username(username, sizeof(username));
    get_broker_password(password, sizeof(password));

    build_topics();

    ESP_LOGW(TAG, "Próba połączenia z: %s", broker_url);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_url,
    };
    
    if (username[0] != 0) mqtt_cfg.credentials.username = username;
    if (password[0] != 0) mqtt_cfg.credentials.authentication.password = password;

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event_handler, NULL);
    
    esp_mqtt_client_start(client);
    s_mqtt_started = true;
}

static void process_sensor_data(const char *topic, double value, const char *unit, enum Parameter param_type) {
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "value", value);

    long timestamp = get_timestamp_seconds();
    cJSON_AddNumberToObject(root, "timestamp", timestamp);

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
            ESP_LOGE(TAG, "Błąd wysyłania. Zapisuję do NVS.");
            save_message_to_nvs(json_str, param_type);
        }
    } else {
        ESP_LOGW(TAG, "Brak połączenia MQTT. Zapisuję do NVS: %s", topic);
        save_message_to_nvs(json_str, param_type);
    }

    free(json_str);
}

void mqtt_send_sensor_data(int soil, float temp, float press) {
    process_sensor_data(topic_soil, (double)soil, "%", SOIL_HUMIDITY);
    process_sensor_data(topic_temp, (double)temp, "C", TEMPERATURE);
    process_sensor_data(topic_press, (double)press, "hPa", PRESSURE);
}

void mqtt_send_coin_event(void) {
    process_sensor_data(topic_coin, 1.0, "PLN", COIN_INSERTED);
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