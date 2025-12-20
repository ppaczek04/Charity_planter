#include "bmp280.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BMP280";

#define REG_DIG_T1    0x88
#define REG_ID        0xD0
#define REG_RESET     0xE0
#define REG_STATUS    0xF3
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG    0xF5
#define REG_PRESS_MSB 0xF7


static esp_err_t write_reg8(bmp280_t *dev, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(dev->port, dev->i2c_addr, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t read_regs(bmp280_t *dev, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(dev->port, dev->i2c_addr, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

// Kompensacja temperatury
static int32_t compensate_temp(bmp280_t *dev, int32_t adc_T) {
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)dev->calib.dig_T1 << 1))) * ((int32_t)dev->calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dev->calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->calib.dig_T1))) >> 12) * ((int32_t)dev->calib.dig_T3)) >> 14;
    dev->t_fine = var1 + var2;
    return (dev->t_fine * 5 + 128) >> 8;
}

// Kompensacja ciśnienia
static uint32_t compensate_pressure(bmp280_t *dev, int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)dev->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dev->calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)dev->calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)dev->calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dev->calib.dig_P3) >> 8) + ((var1 * (int64_t)dev->calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dev->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dev->calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib.dig_P7) << 4);
    return (uint32_t)p;
}

// --- API Publiczne ---

esp_err_t bmp280_init(bmp280_t *dev, i2c_port_t port, uint8_t addr) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    dev->port = port;
    dev->i2c_addr = addr;

    // Sprawdzenie ID
    uint8_t id;
    if (read_regs(dev, REG_ID, &id, 1) != ESP_OK) return ESP_FAIL;
    if (id != BMP280_CHIP_ID && id != 0x60) {
        ESP_LOGE(TAG, "Wrong ID: 0x%02X", id);
        return ESP_FAIL;
    }

    // Reset
    write_reg8(dev, REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Kalibracja
    uint8_t cal[24];
    if (read_regs(dev, REG_DIG_T1, cal, 24) != ESP_OK) return ESP_FAIL;

    dev->calib.dig_T1 = (uint16_t)(cal[1] << 8) | cal[0];
    dev->calib.dig_T2 = (int16_t)((cal[3] << 8) | cal[2]);
    dev->calib.dig_T3 = (int16_t)((cal[5] << 8) | cal[4]);
    dev->calib.dig_P1 = (uint16_t)(cal[7] << 8) | cal[6];
    dev->calib.dig_P2 = (int16_t)((cal[9] << 8) | cal[8]);
    dev->calib.dig_P3 = (int16_t)((cal[11] << 8) | cal[10]);
    dev->calib.dig_P4 = (int16_t)((cal[13] << 8) | cal[12]);
    dev->calib.dig_P5 = (int16_t)((cal[15] << 8) | cal[14]);
    dev->calib.dig_P6 = (int16_t)((cal[17] << 8) | cal[16]);
    dev->calib.dig_P7 = (int16_t)((cal[19] << 8) | cal[18]);
    dev->calib.dig_P8 = (int16_t)((cal[21] << 8) | cal[20]);
    dev->calib.dig_P9 = (int16_t)((cal[23] << 8) | cal[22]);

    ESP_LOGI(TAG, "Initialized");
    return ESP_OK;
}

esp_err_t bmp280_read_float(bmp280_t *dev, float *temp, float *pres) {
    uint8_t data[6];
    if (read_regs(dev, REG_PRESS_MSB, data, 6) != ESP_OK) return ESP_FAIL;

    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    *temp = (float)compensate_temp(dev, adc_T) / 100.0f;
    *pres = (float)compensate_pressure(dev, adc_P) / 256.0f;
    return ESP_OK;
}


esp_err_t bmp280_set_mode(bmp280_t *dev, uint8_t mode) {
    uint8_t reg;
    if (read_regs(dev, REG_CTRL_MEAS, &reg, 1) != ESP_OK) return ESP_FAIL;
    // Bity [1:0]
    reg = (reg & 0xFC) | (mode & 0x03);
    return write_reg8(dev, REG_CTRL_MEAS, reg);
}

esp_err_t bmp280_set_temp_oversampling(bmp280_t *dev, uint8_t os) {
    uint8_t reg;
    if (read_regs(dev, REG_CTRL_MEAS, &reg, 1) != ESP_OK) return ESP_FAIL;
    // Bity [7:5]
    reg = (reg & 0x1F) | ((os & 0x07) << 5);
    return write_reg8(dev, REG_CTRL_MEAS, reg);
}

esp_err_t bmp280_set_press_oversampling(bmp280_t *dev, uint8_t os) {
    uint8_t reg;
    if (read_regs(dev, REG_CTRL_MEAS, &reg, 1) != ESP_OK) return ESP_FAIL;
    // Bity [4:2]
    reg = (reg & 0xE3) | ((os & 0x07) << 2);
    return write_reg8(dev, REG_CTRL_MEAS, reg);
}

esp_err_t bmp280_set_iir_filter(bmp280_t *dev, uint8_t filter) {
    uint8_t reg;
    if (read_regs(dev, REG_CONFIG, &reg, 1) != ESP_OK) return ESP_FAIL;
    // Bity [4:2]
    reg = (reg & 0xE3) | ((filter & 0x07) << 2);
    return write_reg8(dev, REG_CONFIG, reg);
}

esp_err_t bmp280_set_standby_time(bmp280_t *dev, uint8_t t_sb) {
    uint8_t reg;
    if (read_regs(dev, REG_CONFIG, &reg, 1) != ESP_OK) return ESP_FAIL;
    reg = (reg & 0x1F) | ((t_sb & 0x07) << 5);
    return write_reg8(dev, REG_CONFIG, reg);
}

esp_err_t bmp280_get_status(bmp280_t *dev, uint8_t *status) {
    return read_regs(dev, REG_STATUS, status, 1);
}

esp_err_t bmp280_delete(bmp280_t *dev) {
    if (!dev) return ESP_ERR_INVALID_ARG;

    esp_err_t err = bmp280_set_mode(dev, BMP280_MODE_SLEEP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Nie udalo sie uspic czujnika przed usunieciem");
    }

    dev->i2c_addr = 0;
    dev->port = -1;
    memset(&dev->calib, 0, sizeof(dev->calib));

    ESP_LOGI(TAG, "Urzadzenie usuniete (Struktura wyczyszczona)");
    return ESP_OK;
}
