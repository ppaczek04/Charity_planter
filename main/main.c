#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// --- STAŁE I UUID KONFIGURACJI ---
#define GATTS_TAG "WIFI_PROV"

// Własny Serwis, Charakterystyki i Deskryptor
#define WIFI_CONFIG_SERVICE_UUID    0xFF05
static const uint16_t wifi_config_service_uuid = WIFI_CONFIG_SERVICE_UUID;
#define WIFI_SSID_CHAR_UUID         0xFF06
static const uint16_t wifi_ssid_char_uuid = WIFI_SSID_CHAR_UUID; 
#define WIFI_PASS_CHAR_UUID         0xFF07
static const uint16_t wifi_pass_char_uuid = WIFI_PASS_CHAR_UUID;
#define CHAR_USER_DESCRIPTION_UUID  0x2901
static const uint16_t char_user_description_uuid = CHAR_USER_DESCRIPTION_UUID;

// NVS keys
#define NVS_NAMESPACE "storage"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASS_KEY "pass"

// Konfiguracja Timera i Przycisku
#define PROV_TIMEOUT_SECONDS 45
#define BUTTON_GPIO GPIO_NUM_0
#define BUTTON_POLL_DELAY_MS 50
#define SHORT_PRESS_MS 50 

// Wi-Fi Event Group
const int CONNECTED_BIT = BIT0;
static EventGroupHandle_t s_wifi_event_group;

// Uchwyty dla Serwisu Konfiguracji
static uint16_t wifi_config_service_handle = 0;
static uint16_t ssid_char_handle = 0;
static uint16_t pass_char_handle = 0;
static uint16_t ssid_descr_handle = 0;
static uint16_t pass_descr_handle = 0;

static esp_timer_handle_t provisioning_timer;
static bool is_provisioning_active = false;

// Bufor na tymczasowe dane (do ładowania i zapisywania)
static char g_ssid_buffer[33] = {0};
static char g_pass_buffer[65] = {0};

// Globalne uchwyty do komunikacji GATT
static esp_gatt_if_t gatts_if_global = 0;

// Parametry reklamowania
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static bool wifi_credentials_exist(void);
static esp_err_t save_wifi_credentials(const char *ssid, const char *pass);
static esp_err_t load_wifi_credentials(char *ssid_out, size_t ssid_max, char *pass_out, size_t pass_max);

static void provisioning_timeout_callback(void* arg);
static void start_provisioning_ble(void);
static void stop_provisioning_ble(void);

static void configure_and_connect_wifi(const char *ssid, const char *pass);
static void init_wifi_and_try_connect(void);
static void button_task(void *arg);


// STATYCZNA TABELA ATRYBUTÓW GATT

enum {
    IDX_SVC,          
    // Charakterystyka SSID
    IDX_SSID_CHAR,    
    IDX_SSID_VAL,     
    IDX_SSID_DESCR,   
    // Charakterystyka HASŁO
    IDX_PASS_CHAR,    
    IDX_PASS_VAL,     
    IDX_PASS_DESCR,   

    HRS_IDX_NB,       // Liczba atrybutów (7)
};

// Stałe UUID GATT
static const uint16_t primary_service_uuid = 0x2800; // GATT_DECL_PRIMARY_SERVICE_UUID
static const uint16_t character_declaration_uuid = 0x2803; // GATT_DECL_CHARACTERISTIC_UUID

// Deklaracje charakterystyk
static const uint8_t char_prop_read_write[] = { (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE), 0x00, 0x00 };

// Deskryptory
static const uint8_t ssid_descr_val[] = "Wi-Fi SSID";
static const uint8_t pass_descr_val[] = "Wi-Fi Password";

static esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] =
{
    [IDX_SVC] =
    {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
         sizeof(WIFI_CONFIG_SERVICE_UUID), sizeof(WIFI_CONFIG_SERVICE_UUID), (uint8_t *)&wifi_config_service_uuid}
    },

    [IDX_SSID_CHAR] =
    {
        { ESP_GATT_RSP_BY_APP },
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         sizeof(char_prop_read_write), sizeof(char_prop_read_write), (uint8_t *)char_prop_read_write}
    },

    [IDX_SSID_VAL] = {
        { ESP_GATT_RSP_BY_APP },
        {ESP_UUID_LEN_16, (uint8_t *)&wifi_ssid_char_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        33, sizeof(g_ssid_buffer), (uint8_t *)g_ssid_buffer}
    },

    [IDX_SSID_DESCR] =
    {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_user_description_uuid, ESP_GATT_PERM_READ,
         sizeof(ssid_descr_val), sizeof(ssid_descr_val) - 1, (uint8_t *)ssid_descr_val}
    },

    [IDX_PASS_CHAR] =
    {
        { ESP_GATT_RSP_BY_APP },
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         sizeof(char_prop_read_write), sizeof(char_prop_read_write), (uint8_t *)char_prop_read_write}
    },

    [IDX_PASS_VAL] = {
        { ESP_GATT_RSP_BY_APP },
        {ESP_UUID_LEN_16, (uint8_t *)&wifi_pass_char_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        65, sizeof(g_pass_buffer), (uint8_t *)g_pass_buffer}
    },

    [IDX_PASS_DESCR] =
    {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_user_description_uuid, ESP_GATT_PERM_READ,
         sizeof(pass_descr_val), sizeof(pass_descr_val) - 1, (uint8_t *)pass_descr_val}
    },
};

// Obsługa NVS
static bool wifi_credentials_exist(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) { return false; }
    size_t required_size = 0;
    err = nvs_get_str(handle, WIFI_SSID_KEY, NULL, &required_size);
    nvs_close(handle);
    return (err == ESP_OK || err == ESP_ERR_NVS_VALUE_TOO_LONG);
}

static esp_err_t save_wifi_credentials(const char *ssid, const char *pass)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) { ESP_LOGE(GATTS_TAG, "NVS open failed (%s)", esp_err_to_name(err)); return err; }
    err = nvs_set_str(handle, WIFI_SSID_KEY, ssid);
    if (err == ESP_OK) err = nvs_set_str(handle, WIFI_PASS_KEY, pass);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err == ESP_OK) ESP_LOGI(GATTS_TAG, "Saved Wi-Fi credentials to NVS");
    return err;
}

static esp_err_t load_wifi_credentials(char *ssid_out, size_t ssid_max, char *pass_out, size_t pass_max)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;
    esp_err_t final_err = ESP_OK;

    if (ssid_out) {
        size_t ssid_len = ssid_max;
        esp_err_t res = nvs_get_str(handle, WIFI_SSID_KEY, ssid_out, &ssid_len);
        if (res != ESP_OK && res != ESP_ERR_NVS_NOT_FOUND) final_err = res;
        else if (res == ESP_ERR_NVS_NOT_FOUND) ssid_out[0] = '\0';
    }

    if (pass_out) {
        size_t pass_len = pass_max;
        esp_err_t res = nvs_get_str(handle, WIFI_PASS_KEY, pass_out, &pass_len);
        if (res != ESP_OK && res != ESP_ERR_NVS_NOT_FOUND) final_err = res;
        else if (res == ESP_ERR_NVS_NOT_FOUND) pass_out[0] = '\0';
    }
    nvs_close(handle);
    return final_err;
}

static void provisioning_timeout_callback(void* arg)
{
    ESP_LOGI(GATTS_TAG, "Provisioning timeout (%d s) reached. Stopping BLE advertising.", PROV_TIMEOUT_SECONDS);
    stop_provisioning_ble();
}

static void start_provisioning_ble(void)
{
    ESP_LOGI(GATTS_TAG, "Starting BLE Provisioning...");

    if (wifi_credentials_exist()) {
        if (load_wifi_credentials(g_ssid_buffer, sizeof(g_ssid_buffer), g_pass_buffer, sizeof(g_pass_buffer)) == ESP_OK) {
            ESP_LOGI(GATTS_TAG, "Loaded old credentials (SSID: %s) into buffer for potential update.", g_ssid_buffer);
        } else {
            g_ssid_buffer[0] = '\0';
            g_pass_buffer[0] = '\0';
        }
    } else {
        g_ssid_buffer[0] = '\0';
        g_pass_buffer[0] = '\0';
    }

    ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&adv_params));
    is_provisioning_active = true;

    ESP_ERROR_CHECK(esp_timer_start_once(provisioning_timer, PROV_TIMEOUT_SECONDS * 1000000));
}

static void stop_provisioning_ble(void)
{
    if (!is_provisioning_active) return;
    ESP_LOGI(GATTS_TAG, "Stopping ble provisioning...");

    if (esp_timer_is_active(provisioning_timer)) {
        ESP_ERROR_CHECK(esp_timer_stop(provisioning_timer));
    }

    ESP_ERROR_CHECK(esp_ble_gap_stop_advertising());
    is_provisioning_active = false;
}

static void configure_and_connect_wifi(const char *ssid, const char *pass)
{
    wifi_config_t wifi_config = { .sta = { .scan_method = WIFI_FAST_SCAN, .sort_method = WIFI_CONNECT_AP_BY_SIGNAL } };
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);

    stop_provisioning_ble();

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        // esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        stop_provisioning_ble();
    }
}

static void init_wifi_and_try_connect(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    if (wifi_credentials_exist()) {
        char ssid_buf[33];
        char pass_buf[65];

        if (load_wifi_credentials(ssid_buf, sizeof(ssid_buf), pass_buf, sizeof(pass_buf)) == ESP_OK && ssid_buf[0] && pass_buf[0]) {
            configure_and_connect_wifi(ssid_buf, pass_buf);
            return;
        }
    }

    ESP_ERROR_CHECK(esp_wifi_start());
    start_provisioning_ble();
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if(event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        ESP_LOGI(GATTS_TAG, "BLE advertising data set complete.");
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    gatts_if_global = gatts_if;

    switch(event) {
        case ESP_GATTS_REG_EVT:
            ESP_ERROR_CHECK(esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, 0));
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.num_handle == HRS_IDX_NB) {
                wifi_config_service_handle = param->add_attr_tab.handles[IDX_SVC];
                ssid_char_handle = param->add_attr_tab.handles[IDX_SSID_VAL];
                pass_char_handle = param->add_attr_tab.handles[IDX_PASS_VAL];
                ESP_ERROR_CHECK(esp_ble_gatts_start_service(wifi_config_service_handle));
            }
            break;

        case ESP_GATTS_READ_EVT: 
        if(param->read.handle == ssid_char_handle || param->read.handle == pass_char_handle ||
               param->read.handle == ssid_descr_handle || param->read.handle == pass_descr_handle) {

                esp_gatt_rsp_t rsp = {0};
                rsp.attr_value.handle = param->read.handle;

                if (param->read.handle == ssid_char_handle) {
                    char buffer[33] = {0};
                    load_wifi_credentials(buffer, sizeof(buffer), NULL, 0);
                    if (buffer[0] != '\0') {
                        rsp.attr_value.len = strlen(buffer);
                        memcpy(rsp.attr_value.value, buffer, rsp.attr_value.len);
                        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
                        ESP_LOGI(GATTS_TAG, "Read request: SSID sent.");
                    } else {
                        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_NOT_FOUND, NULL);
                    }

                } else if (param->read.handle == pass_char_handle) {
                    char buffer[65] = {0};
                    load_wifi_credentials(NULL, 0, buffer, sizeof(buffer));
                    if (buffer[0] != '\0') {
                        rsp.attr_value.len = strlen(buffer);
                        memcpy(rsp.attr_value.value, buffer, rsp.attr_value.len);
                        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
                        ESP_LOGI(GATTS_TAG, "Read request: PASS sent.");
                    } else {
                        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_NOT_FOUND, NULL);
                    }

                } else if (param->read.handle == ssid_descr_handle) {
                    const char desc[] = "Wi-Fi SSID";
                    rsp.attr_value.len = sizeof(desc) - 1;
                    memcpy(rsp.attr_value.value, desc, rsp.attr_value.len);
                    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
                    ESP_LOGI(GATTS_TAG, "Read request: SSID descriptor sent.");

                } else if (param->read.handle == pass_descr_handle) {
                    const char desc[] = "Wi-Fi Password";
                    rsp.attr_value.len = sizeof(desc) - 1;
                    memcpy(rsp.attr_value.value, desc, rsp.attr_value.len);
                    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
                    ESP_LOGI(GATTS_TAG, "Read request: PASS descriptor sent.");
                }
            }
        break;
        
        case ESP_GATTS_WRITE_EVT: {
            if (param->write.is_prep) {
                break;
            }

            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(
                    gatts_if,
                    param->write.conn_id,
                    param->write.trans_id,
                    ESP_GATT_OK,
                    NULL
                );
            }

            // SSID
            if (param->write.handle == ssid_char_handle) {
                size_t len = MIN(param->write.len, sizeof(g_ssid_buffer) - 1);
                memcpy(g_ssid_buffer, param->write.value, len);
                g_ssid_buffer[len] = '\0';

                ESP_LOGI(GATTS_TAG, "SSID written: %s", g_ssid_buffer);
            }

            // PASSWORD
            else if (param->write.handle == pass_char_handle) {
                size_t len = MIN(param->write.len, sizeof(g_pass_buffer) - 1);
                memcpy(g_pass_buffer, param->write.value, len);
                g_pass_buffer[len] = '\0';

                ESP_LOGI(GATTS_TAG, "PASS written (len=%d)", (int)len);
            }

            if (g_ssid_buffer[0] != '\0' &&
                g_pass_buffer[0] != '\0') {

                ESP_LOGI(GATTS_TAG, "Both SSID and PASS received - saving to NVS");
                save_wifi_credentials(g_ssid_buffer, g_pass_buffer);
                configure_and_connect_wifi(g_ssid_buffer, g_pass_buffer);
            }

            break;
        }

        case ESP_GATTS_CONNECT_EVT:
            if (esp_timer_is_active(provisioning_timer)) {
                ESP_ERROR_CHECK(esp_timer_stop(provisioning_timer));
            }
        break;
        case ESP_GATTS_DISCONNECT_EVT:
            is_provisioning_active = false;
        break;

        default:
            break;
    }
}

static void button_task(void *arg)
{
    int64_t press_start = 0;
    bool pressed = false;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);

    while (1) {
        int level = gpio_get_level(BUTTON_GPIO);

        if (level == 0 && !pressed) {
            pressed = true;
            press_start = esp_timer_get_time() / 1000;
        } else if (level == 1 && pressed) {
            int64_t duration = (esp_timer_get_time() / 1000) - press_start;
            pressed = false;

            if (duration >= SHORT_PRESS_MS && duration < 5000) {
                EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
                if ((bits & CONNECTED_BIT) == 0){
                    esp_wifi_connect();
                }
                
                if (!is_provisioning_active) 
                    start_provisioning_ble();
                else  
                   stop_provisioning_ble();
            } else if (duration >= 5000) {
                stop_provisioning_ble();
                vTaskDelay(pdMS_TO_TICKS(500));
                if (nvs_flash_erase() == ESP_OK) esp_restart();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_DELAY_MS));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    s_wifi_event_group = xEventGroupCreate();

    const esp_timer_create_args_t timer_args = {
        .callback = &provisioning_timeout_callback,
        .name = "prov_timeout_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &provisioning_timer));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    init_wifi_and_try_connect();

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));

    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };
    ESP_ERROR_CHECK(esp_ble_gap_set_device_name("Charity_planter"));
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));

    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}