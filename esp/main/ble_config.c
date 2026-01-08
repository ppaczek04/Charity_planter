#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "nvs_flash.h"
#include "ble_config.h"

extern esp_err_t save_wifi_ssid(const char *ssid);
extern esp_err_t save_wifi_password(const char *password);
extern esp_err_t save_broker_url(const char *url);
extern esp_err_t save_broker_username(const char *username);
extern esp_err_t save_broker_password(const char *password);

extern esp_err_t get_wifi_ssid(char *ssid, size_t len);
extern esp_err_t get_wifi_password(char *password, size_t len);
extern esp_err_t get_broker_url(char *url, size_t len);
extern esp_err_t get_broker_username(char *username, size_t len);
extern esp_err_t get_broker_password(char *password, size_t len);

#define TAG "BLE_CONFIG"
#define PROV_TIMEOUT_SECONDS 45

static char g_ssid[33] = {0};
static char g_pass[65] = {0};
static char g_url[64] = {0};
static char g_user[32] = {0};
static char g_br_pass[64] = {0};

static esp_timer_handle_t provisioning_timer = NULL;
static bool is_advertising = false; 

static bool s_permanent_mode = false;

enum {
    IDX_SVC,
    IDX_SSID_CHAR, IDX_SSID_VAL, IDX_SSID_DESCR,
    IDX_PASS_CHAR, IDX_PASS_VAL, IDX_PASS_DESCR,
    IDX_URL_CHAR,  IDX_URL_VAL,  IDX_URL_DESCR,
    IDX_USER_CHAR, IDX_USER_VAL, IDX_USER_DESCR,
    IDX_BPASS_CHAR,IDX_BPASS_VAL,IDX_BPASS_DESCR,
    HRS_IDX_NB,
};

#define CONFIG_SVC_UUID     0x00FF
#define CHAR_SSID_UUID      0xFF01
#define CHAR_PASS_UUID      0xFF02
#define CHAR_URL_UUID       0xFF03
#define CHAR_USER_UUID      0xFF04
#define CHAR_BPASS_UUID     0xFF05

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t char_decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t char_descr_uuid = ESP_GATT_UUID_CHAR_DESCRIPTION;
static const uint16_t GATTS_SERVICE_UUID_VAL = CONFIG_SVC_UUID;
static const uint16_t GATTS_CHAR_SSID_UUID_VAL = CHAR_SSID_UUID;
static const uint16_t GATTS_CHAR_PASS_UUID_VAL = CHAR_PASS_UUID;
static const uint16_t GATTS_CHAR_URL_UUID_VAL = CHAR_URL_UUID;
static const uint16_t GATTS_CHAR_USER_UUID_VAL = CHAR_USER_UUID;
static const uint16_t GATTS_CHAR_BPASS_UUID_VAL = CHAR_BPASS_UUID;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;

static const uint8_t desc_ssid[] = "Wi-Fi SSID";
static const uint8_t desc_pass[] = "Wi-Fi Password";
static const uint8_t desc_url[]  = "MQTT Broker URL";
static const uint8_t desc_user[] = "MQTT Username";
static const uint8_t desc_bpass[]= "MQTT Password";

static esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] = {
    [IDX_SVC] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(GATTS_SERVICE_UUID_VAL), sizeof(GATTS_SERVICE_UUID_VAL), (uint8_t *)&GATTS_SERVICE_UUID_VAL}},
    
    [IDX_SSID_CHAR]  = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_write}},
    [IDX_SSID_VAL]   = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_SSID_UUID_VAL, ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ, sizeof(g_ssid), sizeof(g_ssid), (uint8_t *)g_ssid}},
    [IDX_SSID_DESCR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_descr_uuid, ESP_GATT_PERM_READ, sizeof(desc_ssid), sizeof(desc_ssid), (uint8_t *)desc_ssid}},

    [IDX_PASS_CHAR]  = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_write}},
    [IDX_PASS_VAL]   = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_PASS_UUID_VAL, ESP_GATT_PERM_WRITE, sizeof(g_pass), sizeof(g_pass), (uint8_t *)g_pass}},
    [IDX_PASS_DESCR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_descr_uuid, ESP_GATT_PERM_READ, sizeof(desc_pass), sizeof(desc_pass), (uint8_t *)desc_pass}},

    [IDX_URL_CHAR]   = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_write}},
    [IDX_URL_VAL]    = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_URL_UUID_VAL, ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ, sizeof(g_url), sizeof(g_url), (uint8_t *)g_url}},
    [IDX_URL_DESCR]  = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_descr_uuid, ESP_GATT_PERM_READ, sizeof(desc_url), sizeof(desc_url), (uint8_t *)desc_url}},

    [IDX_USER_CHAR]  = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_write}},
    [IDX_USER_VAL]   = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_USER_UUID_VAL, ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ, sizeof(g_user), sizeof(g_user), (uint8_t *)g_user}},
    [IDX_USER_DESCR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_descr_uuid, ESP_GATT_PERM_READ, sizeof(desc_user), sizeof(desc_user), (uint8_t *)desc_user}},

    [IDX_BPASS_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_write}},
    [IDX_BPASS_VAL]  = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_BPASS_UUID_VAL, ESP_GATT_PERM_WRITE, sizeof(g_br_pass), sizeof(g_br_pass), (uint8_t *)g_br_pass}},
    [IDX_BPASS_DESCR]= {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_descr_uuid, ESP_GATT_PERM_READ, sizeof(desc_bpass), sizeof(desc_bpass), (uint8_t *)desc_bpass}},
};

uint16_t handles[HRS_IDX_NB];


static void prov_timeout_cb(void* arg) {
    if (s_permanent_mode) return;

    ESP_LOGW(TAG, "Limit czasu 45s minął! Zatrzymywanie rozgłaszania.");
    ble_config_stop();
}

static void check_save_and_reboot(void) {
    if (strlen(g_ssid) > 0 && strlen(g_pass) > 0 && strlen(g_url) > 0) {
        
        save_wifi_ssid(g_ssid);
        save_wifi_password(g_pass);
        save_broker_url(g_url);
        save_broker_username(g_user);
        save_broker_password(g_br_pass);
        
        if (s_permanent_mode) {
            ESP_LOGI(TAG, "Dane zapisane w NVS. Kontynuuję nasłuch BLE");
        } else {
            ESP_LOGI(TAG, "Dane zapisane. Restart za 2s...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
        }
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, 0);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status == ESP_GATT_OK) {
                memcpy(handles, param->add_attr_tab.handles, sizeof(handles));
                esp_ble_gatts_start_service(handles[IDX_SVC]);
            } else {
                ESP_LOGE(TAG, "Create attr table failed, error code = %x", param->add_attr_tab.status);
            }
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "Urządzenie połączone.");
            // W trybie tymczasowym zatrzymujemy timer, żeby dać czas na wpisanie
            if(!s_permanent_mode && provisioning_timer) esp_timer_stop(provisioning_timer);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Rozłączono BLE.");
            // W trybie permanentnym po prostu czekamy dalej. 
            if (!s_permanent_mode) ble_config_stop(); 
            else {
                // W trybie permanentnym wznawiamy advertising po rozłączeniu!
                esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
                    .adv_int_min = 0x20, .adv_int_max = 0x40,
                    .adv_type = ADV_TYPE_IND, .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                    .channel_map = ADV_CHNL_ALL, .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                });
            }
            break;
        case ESP_GATTS_WRITE_EVT: {
            if (param->write.len > 0) {
                char *target = NULL; int max = 0;
                if (handles[IDX_SSID_VAL] == param->write.handle) { target = g_ssid; max = sizeof(g_ssid); }
                else if (handles[IDX_PASS_VAL] == param->write.handle) { target = g_pass; max = sizeof(g_pass); }
                else if (handles[IDX_URL_VAL] == param->write.handle) { target = g_url; max = sizeof(g_url); }
                else if (handles[IDX_USER_VAL] == param->write.handle) { target = g_user; max = sizeof(g_user); }
                else if (handles[IDX_BPASS_VAL] == param->write.handle) { target = g_br_pass; max = sizeof(g_br_pass); }

                if (target) {
                    int len = (param->write.len < max) ? param->write.len : (max - 1);
                    memcpy(target, param->write.value, len);
                    target[len] = '\0';
                }
            }
            if (param->write.need_rsp) esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            
            check_save_and_reboot();
            break;
        }
        default: break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT) {
        esp_ble_adv_params_t adv_params = {
            .adv_int_min = 0x20, .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND, .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL, .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };
        esp_ble_gap_start_advertising(&adv_params);
        is_advertising = true;
        ESP_LOGI(TAG, "Rozpoczęto advertising.");
    }
}

void ble_config_start(bool mode_permanent) {
    if (is_advertising) {
        ESP_LOGW(TAG, "BLE Config już działa! Ignoruję.");
        return;
    }
    
    s_permanent_mode = mode_permanent;

    memset(g_ssid, 0, sizeof(g_ssid));
    memset(g_pass, 0, sizeof(g_pass));
    memset(g_url, 0, sizeof(g_url));
    memset(g_user, 0, sizeof(g_user));
    memset(g_br_pass, 0, sizeof(g_br_pass));

    get_wifi_ssid(g_ssid, sizeof(g_ssid));
    get_wifi_password(g_pass, sizeof(g_pass)); 
    get_broker_url(g_url, sizeof(g_url));
    get_broker_username(g_user, sizeof(g_user));
    get_broker_password(g_br_pass, sizeof(g_br_pass));

    if (provisioning_timer == NULL) {
        const esp_timer_create_args_t timer_args = { .callback = &prov_timeout_cb, .name = "prov_timer" };
        esp_timer_create(&timer_args, &provisioning_timer);
    }

    esp_bt_controller_status_t status = esp_bt_controller_get_status();
    if (status == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        esp_bt_controller_init(&bt_cfg);
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
        esp_bluedroid_init();
        esp_bluedroid_enable();
        esp_ble_gap_register_callback(gap_event_handler);
        esp_ble_gatts_register_callback(gatts_event_handler);
        esp_ble_gatts_app_register(0);
    } else {
        esp_ble_gap_stop_advertising(); 
        uint8_t adv_data[] = { 0x02, 0x01, 0x06, 0x10, 0x09, 'C', 'h', 'a', 'r', 'i', 't', 'y', ' ', 'P', 'l', 'a', 'n', 't', 'e', 'r' };
        esp_ble_gap_config_adv_data_raw(adv_data, sizeof(adv_data));
    }
    
    if (status == ESP_BT_CONTROLLER_STATUS_IDLE) {
         uint8_t adv_data[] = { 0x02, 0x01, 0x06, 0x10, 0x09, 'C', 'h', 'a', 'r', 'i', 't', 'y', ' ', 'P', 'l', 'a', 'n', 't', 'e', 'r' };
         esp_ble_gap_config_adv_data_raw(adv_data, sizeof(adv_data));
    }

    if (!s_permanent_mode) {
        esp_timer_start_once(provisioning_timer, PROV_TIMEOUT_SECONDS * 1000000);
    }
}

void ble_config_stop(void) {
    if (is_advertising) {
        esp_ble_gap_stop_advertising();
        is_advertising = false;
        ESP_LOGI(TAG, "Zatrzymano advertising.");
    }
}