#ifndef NVS_H
#define NVS_H

#include "esp_err.h"
#include "mqtt_client.h"
#include "globals.h"

extern bool is_ssid_set;
extern bool is_password_set;
extern bool is_broker_url_set;
extern bool is_broker_username_set;
extern bool is_broker_password_set;

esp_err_t save_message_to_nvs(const char *message, enum Parameter param);
void resend_messages(const char *namespace_name, const char *topic, esp_mqtt_client_handle_t client);
esp_err_t save_wifi_ssid(const char *ssid);
esp_err_t save_wifi_password(const char *password);
esp_err_t save_broker_url(const char *url);
esp_err_t save_broker_username(const char *username);
esp_err_t save_broker_password(const char *password);
esp_err_t save_owner_id(const char *owner_id);
esp_err_t get_wifi_ssid(char *ssid, size_t ssid_size);
esp_err_t get_wifi_password(char *password, size_t password_size);
esp_err_t get_broker_url(char *url, size_t url_size);
esp_err_t get_broker_username(char *username, size_t username_size);
esp_err_t get_broker_password(char *password, size_t password_size);
esp_err_t get_owner_id(char *owner_id, size_t size);
void load_measurement_interval(void);
esp_err_t save_measurement_interval(uint32_t interval_ms);

#endif 