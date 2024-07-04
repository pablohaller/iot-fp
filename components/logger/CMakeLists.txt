idf_component_register(SRCS "logger.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash cJSON lwip audio
                    )
