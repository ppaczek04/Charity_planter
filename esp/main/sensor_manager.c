#include "sensor_manager.h"
#include "bmp280.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define TAG "SENSOR_MANAGER"

/* --- KONFIGURACJA PINÓW --- */
// I2C (dla BMP280)
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_MASTER_FREQ   100000

// ADC (Wilgotność)
#define SOIL_ADC_CHANNEL  ADC_CHANNEL_0  // GPIO36 (VP)

// GPIO (Wrzutnik monet / Endstop)
#define COIN_PIN          GPIO_NUM_4     // Zakładam pin 4, zmień jeśli masz inny

/* --- ZMIENNE GLOBALNE --- */
static bmp280_t bmp_dev;
static bool bmp_ready = false;
static adc_oneshot_unit_handle_t adc_handle;

/* --- FUNKCJE POMOCNICZE (Lokalne) --- */

static void i2c_bus_init(void) {
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

static void adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,         // Zakres 0-3.3V
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, SOIL_ADC_CHANNEL, &config));
}

static void gpio_coin_init(void) {
    // Konfiguracja pinu monety jako wejście z Pull-up (zakładamy, że endstop zwiera do masy)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // Na razie bez przerwań, sprawdzamy w pętli
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << COIN_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

/* --- FUNKCJE GŁÓWNE (API) --- */

void sensor_manager_init(void) {
    ESP_LOGI(TAG, "Inicjalizacja czujników...");

    // 1. Uruchom I2C i ADC
    i2c_bus_init();
    adc_init();
    gpio_coin_init();

    // 2. Konfiguracja BMP280
    // Używamy Twojej biblioteki bmp280.c
    if (bmp280_init(&bmp_dev, I2C_MASTER_NUM, BMP280_I2C_ADDR_0) == ESP_OK) {
        
        // Ustawienia BMP280 (takie same jak w Twoim pierwszym kodzie)
        bmp280_set_iir_filter(&bmp_dev, BMP280_FILTER_OFF);
        bmp280_set_standby_time(&bmp_dev, BMP280_STANDBY_1000_MS);
        bmp280_set_temp_oversampling(&bmp_dev, BMP280_OS_2X);
        bmp280_set_press_oversampling(&bmp_dev, BMP280_OS_16X);
        bmp280_set_mode(&bmp_dev, BMP280_MODE_NORMAL);
        
        bmp_ready = true;
        ESP_LOGI(TAG, "BMP280 zainicjalizowany poprawnie.");
    } else {
        ESP_LOGE(TAG, "Błąd inicjalizacji BMP280!");
        bmp_ready = false;
    }

    ESP_LOGI(TAG, "Inicjalizacja zakończona.");
}

/* Odczyt wilgotności w procentach */
int sensor_manager_read_soil_percent(void) {
    int raw = 0;
    if (adc_oneshot_read(adc_handle, SOIL_ADC_CHANNEL, &raw) == ESP_OK) {
        // Prosta kalibracja: 0..4095 -> 100%..0%
        int percent = 100 - (raw * 100 / 4095);
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        return percent;
    }
    return -1; // Błąd
}

/* Odczyt temperatury */
float sensor_manager_read_temperature(void) {
    float temp = 0.0f, press = 0.0f;
    if (bmp_ready) {
        bmp280_read_float(&bmp_dev, &temp, &press);
    }
    return temp;
}

/* Odczyt ciśnienia */
float sensor_manager_read_pressure(void) {
    float temp = 0.0f, press = 0.0f;
    if (bmp_ready) {
        bmp280_read_float(&bmp_dev, &temp, &press);
    }
    return press / 100.0f; // Konwersja Pa -> hPa
}

/* Sprawdzenie monety (zwraca true jeśli wrzucona/wciśnięta) */
bool sensor_manager_is_coin_inserted(void) {
    // Jeśli używasz pull-up, wciśnięcie daje stan niski (0)
    return gpio_get_level(COIN_PIN) == 0;
}