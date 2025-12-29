#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_adc/adc_oneshot.h"

// >>> ZMIANA (BMP280) - nowe include
#include "driver/i2c.h"
#include "bmp280.h"
#include "esp_netif.h"
#include <stdbool.h>
////

/* --- KONFIGURACJA --- */
#define WIFI_SSID        "A56Pio"
#define WIFI_PASS        "siema123"
#define MQTT_BROKER_URI  "mqtt://10.101.48.49:1883"   // IP twojego laptopa z Mosquitto
#define TAG              "MQTT_SOIL"

#define CZUJNIK_WILGOTNOSCI_CHANNEL ADC_CHANNEL_0  // GPIO36 (VP)


// >>> ZMIANA (BMP280) - konfiguracja I2C
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_MASTER_FREQ   100000
////

/* --- GLOBALNE --- */
esp_mqtt_client_handle_t mqtt_client;


// >>> ZMIANA (BMP280) - globalny obiekt czujnika
static bmp280_t bmp_dev;
static bool bmp_ok = false;
////

/* --- Wi-Fi --- */
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "🔌 Łączenie z WiFi...");
    esp_wifi_connect();
}

/* --- MQTT --- */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ Połączono z brokerem MQTT");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "⚠️ Rozłączono z brokerem MQTT");
            break;
        default:
            break;
    }
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// >>> ZMIANA (BMP280) - init I2C
static void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ,
        .clk_flags = 0,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}

// >>> ZMIANA (BMP280) - init BMP280
static void bmp280_setup(void)
{
    i2c_master_init();

    esp_err_t res = bmp280_init(&bmp_dev, I2C_MASTER_NUM, BMP280_I2C_ADDR_0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "❌ BMP280 init nieudany (sprawdź kable / adres 0x76 vs 0x77)");
        bmp_ok = false;
        return;
    }

    bmp280_set_iir_filter(&bmp_dev, BMP280_FILTER_OFF);
    bmp280_set_standby_time(&bmp_dev, BMP280_STANDBY_1000_MS);
    bmp280_set_temp_oversampling(&bmp_dev, BMP280_OS_2X);
    bmp280_set_press_oversampling(&bmp_dev, BMP280_OS_16X);
    bmp280_set_mode(&bmp_dev, BMP280_MODE_NORMAL);

    bmp_ok = true;
    ESP_LOGI(TAG, "✅ BMP280 gotowy (temperatura + ciśnienie)");
}
////

/* --- Zadanie odczytu wilgotności gleby --- */
static void soil_moisture_task(void *pvParameter)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);
    // ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,          // Zakres 0–3.3V
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12-bit
    };
    adc_oneshot_config_channel(adc_handle, CZUJNIK_WILGOTNOSCI_CHANNEL, &config);
    // ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CZUJNIK_WILGOTNOSCI_CHANNEL, &config));

    int raw_value;
    // >>> ZMIANA (MQTT TEMP/PRESS) - większy bufor na JSON
    char payload[160];

    ESP_LOGI(TAG, "🌱 Start pomiarów: wilgotność + BMP280 (temp + ciśnienie)");

	while (1) {
		// --- Odczyt wilgotności (ADC) ---
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, CZUJNIK_WILGOTNOSCI_CHANNEL, &raw_value));

		// Prosta konwersja na procent (możesz później skalibrować "sucho/mokro")
		int wilgotnosc_proc = 100 - (raw_value * 100 / 4095);

		// --- Odczyt BMP280 ---
		float temp_c = 0.0f;
		float pres_pa = 0.0f;
		float pres_hpa = 0.0f;
		bool bmp_read_ok = false;

		if (bmp_ok && (bmp280_read_float(&bmp_dev, &temp_c, &pres_pa) == ESP_OK)) {
			pres_hpa = pres_pa / 100.0f;
			bmp_read_ok = true;

			ESP_LOGI(TAG, "🌱 %d%% | 🌡️ %.2f C | 🧭 %.2f hPa",
					 wilgotnosc_proc, temp_c, pres_hpa);
		} else {
			ESP_LOGW(TAG, "⚠️ BMP280: brak odczytu (wysyłam tylko wilgotność)");
			ESP_LOGI(TAG, "🌱 %d%%", wilgotnosc_proc);
		}

		// --- Budowa JSON payload ---
		if (bmp_read_ok) {
			snprintf(payload, sizeof(payload),
					 "{\"soil_raw\":%d,\"soil_percent\":%d,\"temperature_c\":%.2f,\"pressure_hpa\":%.2f}",
					 raw_value, wilgotnosc_proc, temp_c, pres_hpa);
		} else {
			// fallback: bez temp/pressure
			snprintf(payload, sizeof(payload),
					 "{\"soil_raw\":%d,\"soil_percent\":%d}",
					 raw_value, wilgotnosc_proc);
		}

		// --- Publikacja MQTT ---
		if (mqtt_client) {
			esp_mqtt_client_publish(mqtt_client, "sensors/esp32_1/soil", payload, 0, 1, 0);
			ESP_LOGI(TAG, "📤 Wysłano: %s", payload);
		} else {
			ESP_LOGW(TAG, "⚠️ mqtt_client == NULL (nie wysłano)");
		}

		vTaskDelay(pdMS_TO_TICKS(5000)); // co 5 sekund
	}
}

/* --- main() --- */
void app_main(void)
{
    // Inicjalizacja pamięci NVS (konieczna dla WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    mqtt_init();

    // >>> ZMIANA (BMP280) - init BMP280 przed startem taska
    bmp280_setup();

    // Uruchom zadanie odczytu i publikacji wilgotności
    xTaskCreate(soil_moisture_task, "soil_moisture_task", 4096, NULL, 5, NULL);
}
