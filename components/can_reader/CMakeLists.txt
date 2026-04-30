idf_component_register(SRCS "can_reader.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${PROJECT_DIR}"
                    REQUIRES driver freertos dbc_schema esp_timer)
