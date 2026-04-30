idf_component_register(SRCS "http_api.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${PROJECT_DIR}"
                    REQUIRES esp_http_server json can_reader esp_netif)
