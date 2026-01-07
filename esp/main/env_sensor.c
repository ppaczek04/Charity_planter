#include "env_sensor.h"
#include "bmp280.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "BMP280"
#define SDA_GPIO 21
#define SCL_GPIO 22

static bmp280_t dev;
static bool is_init = false;

void env_sensor_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_GPIO, 
        .scl_io_num = SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE, 
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    if (bmp280_init(&dev, I2C_NUM_0, BMP280_I2C_ADDR_0) == ESP_OK) {
        bmp280_set_temp_oversampling(&dev, BMP280_OS_2X);
        
        bmp280_set_press_oversampling(&dev, BMP280_OS_16X);
        
        bmp280_set_iir_filter(&dev, BMP280_FILTER_OFF);
        
        bmp280_set_mode(&dev, BMP280_MODE_SLEEP);
        
        is_init = true;
        ESP_LOGI(TAG, "Zainicjalizowano BMP280 w trybie FORCED (Energy Saving)");
    } else {
        ESP_LOGE(TAG, "Błąd inicjalizacji BMP280");
    }
}

bool env_sensor_read(float *temp, float *pressure) {
    if (!is_init) return false;
    
    esp_err_t err = bmp280_set_mode(&dev, BMP280_MODE_FORCED);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Nie udalo sie wymusic pomiaru");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    float p_pa;
    if (bmp280_read_float(&dev, temp, &p_pa) == ESP_OK) {
        *pressure = p_pa / 100.0f; // konwersja Pa -> hPa
        return true;
    }
    
    return false;
}