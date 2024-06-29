idf_component_register(SRCS "webserver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server
                    EMBED_FILES "foo.html"
                )
