idf_component_register(SRCS "main.c"
                        "wifi_ap.c"
                        "http_get_task.c"
                        "wifi_sta.c"
                    PRIV_REQUIRES esp_wifi nvs_flash driver esp_netif log esp_event bt
                    INCLUDE_DIRS ".")
