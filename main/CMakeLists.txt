idf_component_register(
    SRCS 
        "station_example_main.c"
        "http_get_task.c"
        "wifi_ap.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt esp_wifi esp_event nvs_flash driver
)