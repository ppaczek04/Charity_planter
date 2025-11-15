#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_ap.h" 
#include "wifi_sta.h"
#include "http_get_task.h"

static const char *TAG_MAIN = "MAIN";

// tryb station - 0, tryb ap - 1
#define WIFI_MODE_AP_ENABLE 0

void app_main(void)
{
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (WIFI_MODE_AP_ENABLE) {
        ESP_LOGI(TAG_MAIN, "STARTING: WIFI MODE AP");
        wifi_init_softap(); 
    } else {
        s_wifi_event_group = xEventGroupCreate();
        if (s_wifi_event_group != NULL) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTING_BIT);
        }

        ESP_LOGI(TAG_MAIN, "STARTING: WIFI MODE STA");
        xTaskCreate(blink_led_task, "blink_led_task", 2048, NULL, 5, NULL);
        wifi_init_sta();
        xTaskCreate(http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    }
}