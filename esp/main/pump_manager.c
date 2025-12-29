#include "pump_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "PUMP"

// KONFIGURACJA PINÓW
#define PUMP_PIN      GPIO_NUM_2  // Przekaźnik pompki (zmień na swój)
#define COIN_PIN      GPIO_NUM_4  // Endstop monety

void pump_manager_init(void) {
    // 1. Konfiguracja Pompki (Wyjście)
    gpio_reset_pin(PUMP_PIN);
    gpio_set_direction(PUMP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PUMP_PIN, 0); // Domyślnie wyłączona

    // 2. Konfiguracja Monety (Wejście z Pull-up)
    gpio_reset_pin(COIN_PIN);
    gpio_set_direction(COIN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(COIN_PIN, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "Pompka i wrzutnik zainicjalizowane.");
}

void pump_turn_on(void) {
    ESP_LOGI(TAG, "Uruchamiam podlewanie!");
    gpio_set_level(PUMP_PIN, 1);
}

void pump_turn_off(void) {
    ESP_LOGI(TAG, "Koniec podlewania.");
    gpio_set_level(PUMP_PIN, 0);
}

bool is_coin_inserted(void) {
    // Jeśli pull-up, to wciśnięcie zwiera do masy (stan 0)
    return gpio_get_level(COIN_PIN) == 0;
}