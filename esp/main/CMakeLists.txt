idf_component_register(
    SRCS "main.c" "globals.c" "sensor_manager.c" "bmp280.c" "pump_manager.c" "mqtt.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash esp_system esp_netif mqtt driver esp_adc
)