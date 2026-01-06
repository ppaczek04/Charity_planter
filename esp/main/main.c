#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Moduły
#include "endstop_manager.h"
#include "nvs.h"    // NVS (obsługa pamięci)
#include "soil_sensor.h"    // Twój czujnik wilgotności
#include "env_sensor.h"     // Twój BMP280
#include "mqtt_manager.h"   // Wysyłanie danych
#include "ble_config.h"     // Konfiguracja przez telefon
#include "wifi_manager.h"   // <--- NOWY PLIK: Obsługa WiFi
#include "pump_manager.h"   // Zarządzanie pompką

#define TAG "MAIN"
#define BUTTON_GPIO 0       // Przycisk BOOT

// Callback do obsługi komendy podlewania z MQTT
static void handle_water_command(float duration_sec) {
    ESP_LOGI(TAG, "🚿 Włączam pompkę na %.1f sekundy!", duration_sec);
    
    pump_turn_on();
    vTaskDelay(pdMS_TO_TICKS((int)(duration_sec * 1000)));
    pump_turn_off();
    
    ESP_LOGI(TAG, "✅ Podlewanie zakończone");
}

void app_main(void) {
    // 1. Inicjalizacja NVS (Pamięć trwała)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. Sprawdzenie przycisku (Wejście w tryb konfiguracji)
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    vTaskDelay(pdMS_TO_TICKS(100)); // Krótka chwila na stabilizację

    if (gpio_get_level(BUTTON_GPIO) == 0) {
        ESP_LOGI(TAG, ">>> TRYB KONFIGURACJI BLE URUCHOMIONY <<<");
        ble_config_start();
        
        // Pętla nieskończona - w trybie config urządzenie tylko czeka na dane.
        // Po wgraniu danych użytkownik musi zrestartować ESP32.
        while(1) vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // 3. Tryb Normalny (Praca)
    ESP_LOGI(TAG, "Start systemu w trybie normalnym...");

    // Inicjalizacja czujników
    soil_sensor_init();
    env_sensor_init();
    pump_manager_init();

    // Sprawdzamy czy mamy w ogóle konfigurację WiFi zapisaną w pamięci
    char check_ssid[32];
    if (get_wifi_ssid(check_ssid, sizeof(check_ssid)) == ESP_OK) {
        
        // Mamy konfigurację -> Uruchamiamy WiFi i MQTT
        wifi_manager_init();
        mqtt_manager_init();
        
        // Rejestrujemy callback do obsługi komend podlewania
        set_water_command_callback(handle_water_command);
        
        endstop_manager_init();
    } else {
        ESP_LOGE(TAG, "BRAK KONFIGURACJI WIFI! Przytrzymaj przycisk BOOT przy starcie.");
    }

    // 4. Pętla główna (Loop)
    while (1) {
        // Pobierz dane
        int soil = soil_sensor_get_percentage();
        float temp = 0.0f, press = 0.0f;
        env_sensor_read(&temp, &press);

        // Logowanie
        ESP_LOGI(TAG, "Pomiary: Gleba %d%%, Temp %.2f C, Cisnienie %.2f hPa", soil, temp, press);
        
        // Wysyłanie (MQTT Manager sam sprawdzi czy jest połączenie zanim wyśle)
        if (wifi_manager_is_connected()) {
             mqtt_send_sensor_data(soil, temp, press);
        } else {
             ESP_LOGW(TAG, "Brak WiFi - dane nie wysłane");
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Czekaj 5 sekund
    }
}