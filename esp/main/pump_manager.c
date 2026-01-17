#include "pump_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "PUMP"

#define PUMP_PIN      GPIO_NUM_18  // Przekaźnik pompki

void pump_manager_init(void) {
    // Konfiguracja Pompki (Wyjście)
    gpio_reset_pin(PUMP_PIN);
    gpio_set_direction(PUMP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PUMP_PIN, 1); // Domyślnie wyłączona (HIGH = OFF dla przekaźnika low-trigger)

    ESP_LOGI(TAG, "Pompka zainicjalizowana.");
}

void pump_turn_on(void) {
    ESP_LOGI(TAG, "Uruchamiam podlewanie!");
    gpio_set_level(PUMP_PIN, 0); // LOW = ON dla przekaźnika low-trigger
}

void pump_turn_off(void) {
    ESP_LOGI(TAG, "Koniec podlewania.");
    gpio_set_level(PUMP_PIN, 1); // HIGH = OFF dla przekaźnika low-trigger
}