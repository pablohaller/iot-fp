idf_component_register(
    SRCS "webserver.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server cJSON network logger mqttclient config
    EMBED_FILES "foo.html"
)
