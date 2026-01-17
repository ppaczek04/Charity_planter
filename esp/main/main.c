#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs.h"    
#include "soil_sensor.h"   
#include "env_sensor.h"     
#include "mqtt_manager.h"   
#include "wifi_manager.h"   
#include "pump_manager.h"  
#include "ble_config.h" 
#include "endstop_manager.h"

#define TAG "MAIN"
#define BUTTON_GPIO 0 

static QueueHandle_t water_cmd_queue = NULL;

extern esp_err_t get_wifi_ssid(char *ssid, size_t ssid_size);
extern esp_err_t get_broker_url(char *url, size_t url_size);
extern uint32_t g_measurement_interval_ms;
extern void load_measurement_interval(void);

static void handle_water_command(float duration_sec) {
    if (water_cmd_queue != NULL) {
        if (xQueueSend(water_cmd_queue, &duration_sec, 0) == pdTRUE) {
            ESP_LOGI(TAG, "Komenda podlewania dodana do kolejki (%.1fs)", duration_sec);
        } else {
            ESP_LOGW(TAG, "Kolejka podlewania pełna! Ignoruję komendę.");
        }
    }
}

void pump_worker_task(void *arg) {
    float duration;

    while (1) {
        if (xQueueReceive(water_cmd_queue, &duration, portMAX_DELAY)) {
            
            ESP_LOGI(TAG, "ROZPOCZYNAM PODLEWANIE Z KOLEJKI (%.1fs)", duration);
            
            pump_turn_on();
            
            vTaskDelay(pdMS_TO_TICKS((int)(duration * 1000)));
            
            pump_turn_off();
            vTaskDelay(pdMS_TO_TICKS(2500)); 
        }
    }
}

void button_watch_task(void *arg) {
    bool already_pressed = false;
    while (1) {
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50)); 
            if (gpio_get_level(BUTTON_GPIO) == 0 && !already_pressed) {
                ESP_LOGW(TAG, "TRYB SZYBKIEJ EDYCJI (Czasowe BLE)");
                ble_config_start(false); 
                already_pressed = true;
            }
        } else {
            already_pressed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void sensor_reading_task(void *arg) {
    while (1) {
        int soil = soil_sensor_get_percentage();
        float temp = 0.0f, press = 0.0f;
        env_sensor_read(&temp, &press);

        mqtt_send_sensor_data(soil, temp, press);

        vTaskDelay(pdMS_TO_TICKS(g_measurement_interval_ms));
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    load_measurement_interval();
    water_cmd_queue = xQueueCreate(10, sizeof(float));
    if (water_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Błąd tworzenia kolejki!");
        return;
    }

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    if (gpio_get_level(BUTTON_GPIO) == 0) {
        ESP_LOGW(TAG, "TRYB GŁÓWNEJ KONFIGURACJI (Permanentny)");
        ble_config_start(true);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "START SYSTEMU (Tryb Normalny)");

    char check_ssid[32] = {0};
    if (get_wifi_ssid(check_ssid, sizeof(check_ssid)) != ESP_OK || strlen(check_ssid) == 0) {
        ESP_LOGE(TAG, "BRAK DANYCH WIFI! Kliknij przycisk BOOT, aby skonfigurować.");
    }

    soil_sensor_init();
    env_sensor_init();
    pump_manager_init();
    endstop_manager_init(); 
    
    wifi_manager_init();
    mqtt_manager_init();

    set_water_command_callback(handle_water_command);

    xTaskCreate(button_watch_task, "btn_task", 2048, NULL, 10, NULL);
    xTaskCreate(sensor_reading_task, "sensor_task", 4096, NULL, 5, NULL);
    
    xTaskCreate(pump_worker_task, "pump_worker", 2048, NULL, 5, NULL);

    vTaskDelete(NULL);
}