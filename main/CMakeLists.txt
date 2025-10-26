idf_component_register(SRCS "station_example_main.c"
                            "http_get_task.c"
                            "wifi_ap.c"
                    PRIV_REQUIRES esp_wifi nvs_flash driver esp_netif
                    INCLUDE_DIRS ".")