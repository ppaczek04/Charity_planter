#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "globals.h"

#define MAX_MESSAGES 10
#define NVS "NVS"
#define MQTT "MQTT_NVS"

bool is_ssid_set = false;
bool is_password_set = false;
bool is_broker_url_set = false;
bool is_broker_username_set = false;
bool is_broker_password_set = false;

const char* get_namespace_name(enum Parameter param) {
    switch (param) {
        case TEMPERATURE:
            return NVS_NS_TEMP;
        case SOIL_HUMIDITY:
            return NVS_NS_SOIL;
        case PRESSURE:
            return NVS_NS_PRESS;
        case COIN_INSERTED:
            return NVS_NS_COIN;
        default:
            ESP_LOGE(NVS, "Invalid parameter provided: %d", param);
            return NULL; 
    }
}

esp_err_t save_message_to_nvs(const char *message, enum Parameter param) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    const char *namespace_name = get_namespace_name(param);

    err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(namespace_name, "Could not open buffer: %s", namespace_name);
        return err;
    }

    for (int i = 0; i < MAX_MESSAGES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "msg_%d", i);

        size_t required_size;
        err = nvs_get_str(nvs_handle, key, NULL, &required_size);

        if (err == ESP_ERR_NVS_NOT_FOUND) {
            err = nvs_set_str(nvs_handle, key, message);
            if (err == ESP_OK) {
                nvs_commit(nvs_handle);
            }
            nvs_close(nvs_handle);
            return err;
        }
    }

    ESP_LOGW(namespace_name, "Buffer full - cannot save more messages.");
    nvs_close(nvs_handle);
    return ESP_ERR_NO_MEM;
}

void resend_messages(const char *namespace_name, const char *topic, esp_mqtt_client_handle_t client) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        return;
    }

    for (int i = 0; i < MAX_MESSAGES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "msg_%d", i);

        size_t required_size;
        err = nvs_get_str(nvs_handle, key, NULL, &required_size);

        if (err == ESP_OK) {
            char *message = malloc(required_size);
            if (message) {
                err = nvs_get_str(nvs_handle, key, message, &required_size);
                if (err == ESP_OK) {
                    int msg_id = esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
                    if (msg_id != -1) {
                        ESP_LOGI(MQTT, "Successfully sent message");
                        nvs_erase_key(nvs_handle, key);
                        nvs_commit(nvs_handle); 
                    } else {
                        ESP_LOGE(MQTT, "Could not resent message.");
                    }
                }
                free(message); 
            }
        } else {
            break; 
        }
    }

    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

void resend_messages_from_nvs(esp_mqtt_client_handle_t client) {
    ESP_LOGI("MQTT", "Rozpoczynam wysyłanie danych offline z NVS...");

    resend_messages(NVS_NS_SOIL, TOPIC_SOIL, client);
    resend_messages(NVS_NS_TEMP, TOPIC_TEMP, client);
    resend_messages(NVS_NS_PRESS, TOPIC_PRESS, client);
    resend_messages(NVS_NS_COIN, TOPIC_COIN, client);
}

esp_err_t save_wifi_ssid(const char *ssid) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("wifi_config", NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, "ssid", ssid));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
    is_ssid_set = true;

    return ESP_OK;
}

esp_err_t save_wifi_password(const char *password) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("wifi_config", NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, "password", password));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
    is_password_set = true;

    return ESP_OK;
}

esp_err_t save_broker_url(const char *url) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("broker_config", NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, "url", url));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
    is_broker_url_set = true;

    return ESP_OK;
}

esp_err_t save_broker_username(const char *username) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("broker_config", NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, "username", username));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
    is_broker_username_set = true;

    return ESP_OK;
}

esp_err_t save_broker_password(const char *password) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("broker_config", NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_str(my_handle, "password", password));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
    is_broker_password_set = true;

    return ESP_OK;
}

esp_err_t get_wifi_ssid(char *ssid, size_t ssid_size) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("wifi_config", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_str(my_handle, "ssid", ssid, &ssid_size);
    nvs_close(my_handle);
    return ret;
}

esp_err_t get_wifi_password(char *password, size_t password_size) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("wifi_config", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_str(my_handle, "password", password, &password_size);
    nvs_close(my_handle);
    return ret;
}

esp_err_t get_broker_url(char *url, size_t url_size) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("broker_config", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_str(my_handle, "url", url, &url_size);
    nvs_close(my_handle);
    return ret;
}

esp_err_t get_broker_username(char *username, size_t username_size) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("broker_config", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_str(my_handle, "username", username, &username_size);
    nvs_close(my_handle);
    return ret;
}

esp_err_t get_broker_password(char *password, size_t password_size) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("broker_config", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = nvs_get_str(my_handle, "password", password, &password_size);
    nvs_close(my_handle);
    return ret;
}