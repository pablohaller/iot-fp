#include <stdio.h>
#include <esp_http_server.h>
#include <string.h>
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"
#include "webserver.h"

#define TAG "webserver"

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    extern const uint8_t foo_html_start[] asm("_binary_foo_html_start");
    extern const uint8_t foo_html_end[] asm("_binary_foo_html_end");
    const size_t foo_html_size = (foo_html_end - foo_html_start);
    httpd_resp_send(req, (const char *)foo_html_start, foo_html_size);
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = hello_get_handler};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void init_webserver(void)
{
    ESP_LOGI(TAG, "Starting HTTP Server");
    static httpd_handle_t server = NULL;
    server = start_webserver();
}
