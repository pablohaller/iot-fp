idf_component_register(SRCS "mqttclient.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash mqtt
                    )
