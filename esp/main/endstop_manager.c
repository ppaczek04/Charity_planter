#include "endstop_manager.h"
#include "mqtt_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#define TAG "ENDSTOP"

static QueueHandle_t gpio_evt_queue = NULL;

// Zapobiega zaliczeniu monety wielokrotnie - zwiększony debounce ze względu na zakłócenia od pompki
#define DEBOUNCE_TIME_US 2000000  // 2 sekundy debounce

// Zmienna przechowująca czas ostatniego wciśnięcia
static volatile int64_t last_interrupt_time = 0;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    int64_t current_time = esp_timer_get_time();

    if (current_time - last_interrupt_time > DEBOUNCE_TIME_US) {
        last_interrupt_time = current_time;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

static void endstop_task(void* arg) {
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "🪙 Wykryto zdarzenie na GPIO[%d]", io_num);
            
            // Czekamy 150ms na ustabilizowanie sygnału
            vTaskDelay(pdMS_TO_TICKS(150));
            
            // Sprawdzamy czy endstop nadal jest wciśnięty (LOW = moneta)
            if (gpio_get_level(COIN_ENDSTOP_PIN) == 0) {
                ESP_LOGI(TAG, "✅ Potwierdzono wrzut monety!");
                
                // Wysyłamy event monety przez MQTT - backend zdecyduje czy podlewać
                mqtt_send_coin_event();
                
                ESP_LOGI(TAG, "📡 Event wysłany do backendu");
            } else {
                ESP_LOGW(TAG, "⚠️ Fałszywy alarm - endstop nie jest wciśnięty");
            }
        }
    }
}

void endstop_manager_init(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << COIN_ENDSTOP_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(endstop_task, "endstop_task", 2048, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(COIN_ENDSTOP_PIN, gpio_isr_handler, (void*) COIN_ENDSTOP_PIN);

    ESP_LOGI(TAG, "Endstop (Wrzutnik) zainicjalizowany na GPIO %d", COIN_ENDSTOP_PIN);
}