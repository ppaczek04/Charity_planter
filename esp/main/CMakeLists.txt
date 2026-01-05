idf_component_register(
    SRCS "wifi_manager.c" "ble_config.c" "mqtt_manager.c" "env_sensor.c" "soil_sensor.c" "nvs.c" "main.c" "globals.c" "endstop_manager.c" "bmp280.c" "pump_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash 
        bt                
        mqtt 
        driver 
        esp_adc 
        json
        esp_wifi
        esp_netif
)