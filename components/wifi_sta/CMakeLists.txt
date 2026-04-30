idf_component_register(SRCS "wifi_sta.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${PROJECT_DIR}"
                    REQUIRES esp_wifi esp_event nvs_flash)
