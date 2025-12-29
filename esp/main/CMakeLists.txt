idf_component_register(
    SRCS 
        "main.c"
        "bmp280.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash esp_system esp_netif mqtt driver esp_adc
)