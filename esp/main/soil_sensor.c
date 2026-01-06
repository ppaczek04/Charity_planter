#include "soil_sensor.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#define TAG "SOIL"
#define ADC_CHANNEL ADC_CHANNEL_6 // GPIO34
#define ADC_UNIT    ADC_UNIT_1

#define AIR_VALUE   4095
#define WATER_VALUE 1500

static adc_oneshot_unit_handle_t adc_handle = NULL;

void soil_sensor_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));
    ESP_LOGI(TAG, "Zainicjalizowano czujnik wilgotności gleby");
}

int soil_sensor_read_raw(void) {
    int val = 0;
    if (adc_handle) {
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &val);
    }
    return val;
}

int soil_sensor_get_percentage(void) {
    int raw = soil_sensor_read_raw();
    
    // Prosta mapa: (raw - min) * 100 / (max - min)
    // Uwaga: czujniki pojemnościowe często mają odwróconą logikę (powietrze = max volt, woda = min volt)
    if (raw > AIR_VALUE) raw = AIR_VALUE;
    if (raw < WATER_VALUE) raw = WATER_VALUE;

    int percent = 100 - ((raw - WATER_VALUE) * 100 / (AIR_VALUE - WATER_VALUE));
    return percent;
}