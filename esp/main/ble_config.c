#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

// Biblioteki Bluetooth
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

// Obsługa zapisu danych
#include "nvs.h"
#include "ble_config.h" // Plik nagłówkowy dla tego modułu

#define TAG "BLE_CONFIG"

// --- DEFINICJE BLE ---
#define GATTS_SERVICE_UUID      0xABCD
#define GATTS_NUM_HANDLE        15

// UUIDs dla charakterystyk
static esp_bt_uuid_t SSID_CHAR_UUID            = { .len = ESP_UUID_LEN_16, .uuid.uuid16 = 0xFF01 };    
static esp_bt_uuid_t PASSWORD_CHAR_UUID        = { .len = ESP_UUID_LEN_16, .uuid.uuid16 = 0xFF02 }; 
static esp_bt_uuid_t BROKER_URL_CHAR_UUID      = { .len = ESP_UUID_LEN_16, .uuid.uuid16 = 0xFF03 }; 
static esp_bt_uuid_t BROKER_USERNAME_CHAR_UUID = { .len = ESP_UUID_LEN_16, .uuid.uuid16 = 0xFF04 }; 
static esp_bt_uuid_t BROKER_PASSWORD_CHAR_UUID = { .len = ESP_UUID_LEN_16, .uuid.uuid16 = 0xFF05 }; 

// Uchwyty (Handles)
static uint16_t ssid_handle;
static uint16_t password_handle;
static uint16_t broker_url_handle;
static uint16_t broker_username_handle;
static uint16_t broker_password_handle;
static uint16_t service_handle = 0;

// Parametry rozgłaszania
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// --- OBSŁUGA ZDARZEŃ GAP (Rozgłaszanie) ---
static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "Rozpoczęto rozgłaszanie BLE");
            break;
        case ESP_GAP_BLE_SCAN_TIMEOUT_EVT:
            ESP_LOGI(TAG, "Restart rozgłaszania po timeout");
            esp_ble_gap_start_advertising(&adv_params);
            break;
        default:
            break;
    }
}

// --- OBSŁUGA ZDARZEŃ GATT (Zapis/Odczyt danych) ---
static void ble_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatt_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
             esp_ble_gatts_create_service(gatt_if, &(esp_gatt_srvc_id_t) {
                .is_primary = true,
                .id = {.inst_id = 0, .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_SERVICE_UUID}}}
            }, GATTS_NUM_HANDLE);
            break;

        case ESP_GATTS_CREATE_EVT:
            service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(service_handle);

            // Dodawanie charakterystyk do serwisu
            esp_ble_gatts_add_char(service_handle, &SSID_CHAR_UUID, ESP_GATT_PERM_WRITE, ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
            esp_ble_gatts_add_char(service_handle, &PASSWORD_CHAR_UUID, ESP_GATT_PERM_WRITE, ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
            esp_ble_gatts_add_char(service_handle, &BROKER_URL_CHAR_UUID, ESP_GATT_PERM_WRITE, ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
            esp_ble_gatts_add_char(service_handle, &BROKER_USERNAME_CHAR_UUID, ESP_GATT_PERM_WRITE, ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
            esp_ble_gatts_add_char(service_handle, &BROKER_PASSWORD_CHAR_UUID, ESP_GATT_PERM_WRITE, ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
            break;

        case ESP_GATTS_ADD_CHAR_EVT:
            // Przypisywanie uchwytów (handle) do zmiennych, aby wiedzieć gdzie co wpisano
            if (param->add_char.char_uuid.uuid.uuid16 == SSID_CHAR_UUID.uuid.uuid16) ssid_handle = param->add_char.attr_handle;
            else if (param->add_char.char_uuid.uuid.uuid16 == PASSWORD_CHAR_UUID.uuid.uuid16) password_handle = param->add_char.attr_handle;
            else if (param->add_char.char_uuid.uuid.uuid16 == BROKER_URL_CHAR_UUID.uuid.uuid16) broker_url_handle = param->add_char.attr_handle;
            else if (param->add_char.char_uuid.uuid.uuid16 == BROKER_USERNAME_CHAR_UUID.uuid.uuid16) broker_username_handle = param->add_char.attr_handle;
            else if (param->add_char.char_uuid.uuid.uuid16 == BROKER_PASSWORD_CHAR_UUID.uuid.uuid16) broker_password_handle = param->add_char.attr_handle;
            break;

        case ESP_GATTS_WRITE_EVT:
        // 1. Logika odbioru danych (to co masz teraz)
        if (param->write.len < 128) {
            char received_data[129];
            memcpy(received_data, param->write.value, param->write.len);
            received_data[param->write.len] = '\0';

            if (param->write.handle == ssid_handle) {
                ESP_LOGI(TAG, "Otrzymano SSID: %s", received_data);
                save_wifi_ssid(received_data);
            } else if (param->write.handle == password_handle) {
                ESP_LOGI(TAG, "Otrzymano hasło WiFi");
                save_wifi_password(received_data);
            } else if (param->write.handle == broker_url_handle) {
                ESP_LOGI(TAG, "Otrzymano URL Brokera: %s", received_data);
                save_broker_url(received_data);
            } else if (param->write.handle == broker_username_handle) {
                ESP_LOGI(TAG, "Otrzymano login brokera");
                save_broker_username(received_data);
            } else if (param->write.handle == broker_password_handle) {
                ESP_LOGI(TAG, "Otrzymano hasło brokera");
                save_broker_password(received_data);
            }
        } else {
            ESP_LOGW(TAG, "Otrzymano za długi ciąg danych, ignoruję.");
        }

        // 2. KLUCZOWA POPRAWKA: Wysłanie potwierdzenia (Response)
        if (param->write.need_rsp) {
            esp_gatt_rsp_t rsp;
            // Zerujemy strukturę odpowiedzi
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            
            // Przepisujemy uchwyt, żeby telefon wiedział, o którą charakterystykę chodzi
            rsp.attr_value.handle = param->write.handle;
            
            // Opcjonalnie: można odesłać długość 0 lub to co dostaliśmy, 
            // ale standardowo przy Write Request wystarczy status OK.
            rsp.attr_value.len = 0; 

            esp_ble_gatts_send_response(gatt_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, &rsp);
        }
        break;
            
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "Urządzenie połączone po BLE");
            break;
        
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Rozłączono BLE, restart rozgłaszania");
            esp_ble_gap_start_advertising(&adv_params);
            break;

        default:
            break;
    }
}

// --- FUNKCJA INICJALIZUJĄCA (PUBLICZNA) ---
void ble_config_start(void) {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret;

    // Inicjalizacja kontrolera BT
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // Inicjalizacja Bluedroid (stos programowy)
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // Konfiguracja nazwy urządzenia i danych rozgłoszeniowych
    uint8_t adv_data[] = {
        0x02, 0x01, 0x06, 
        0x0A, 0x09, 'S', 'm', 'a', 'r', 't', ' ', 'P', 'o', 't', // Nazwa: "Smart Pot"
        0x03, 0x03, 0xCD, 0xAB 
    };
    esp_ble_gap_config_adv_data_raw(adv_data, sizeof(adv_data));
    
    // Rejestracja callbacków
    esp_ble_gap_register_callback(ble_gap_event_handler);
    esp_ble_gatts_register_callback(ble_gatts_event_handler);
    esp_ble_gatts_app_register(0);
    
    ESP_LOGI(TAG, "BLE Config Service wystartował.");
}