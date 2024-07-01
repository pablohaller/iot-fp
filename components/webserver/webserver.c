#include <stdio.h>
#include <esp_http_server.h>
#include <string.h>
#include <esp_err.h>
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "cJSON.h"
#include "network.h"
#include "webserver.h"
#include "logger.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define TAG "webserver"

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    extern const uint8_t foo_html_start[] asm("_binary_foo_html_start");
    extern const uint8_t foo_html_end[] asm("_binary_foo_html_end");
    const size_t foo_html_size = (foo_html_end - foo_html_start);
    httpd_resp_send(req, (const char *)foo_html_start, foo_html_size);
    buffer_print();
    return ESP_OK;
}

static esp_err_t logs_get_handler(httpd_req_t *req)
{
    char *json_string = buffer_to_json();
    if (json_string == NULL)
    {
        const char *resp = "Error generating JSON";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);

    free(json_string); // Liberar la memoria asignada por cJSON_Print
    return ESP_OK;
}

esp_err_t sta_connect_post_handler(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (recv_size < sizeof(content))
    {
        content[recv_size] = '\0';
    }
    else
    {
        content[sizeof(content) - 1] = '\0';
    }

    ESP_LOGI(TAG, "Received content: %s", content);

    cJSON *json = cJSON_Parse(content);
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        const char resp[] = "Invalid JSON";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    const cJSON *ssid_json = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    const cJSON *password_json = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (cJSON_IsString(ssid_json) && (ssid_json->valuestring != NULL) &&
        cJSON_IsString(password_json) && (password_json->valuestring != NULL))
    {
        ESP_LOGI(TAG, "Parsed SSID: %s", ssid_json->valuestring);
        ESP_LOGI(TAG, "Parsed Password: %s", password_json->valuestring);
        wifi_init_sta();
    }
    else
    {
        ESP_LOGE(TAG, "JSON does not contain expected fields");
        cJSON_Delete(json);
        const char resp[] = "Invalid JSON fields";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    cJSON_Delete(json);

    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = hello_get_handler};

static const httpd_uri_t sta_connect = {
    .uri = "/sta-connect",
    .method = HTTP_POST,
    .handler = sta_connect_post_handler};

static const httpd_uri_t logs = {
    .uri = "/logs",
    .method = HTTP_GET,
    .handler = logs_get_handler};

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
        httpd_register_uri_handler(server, &sta_connect);
        httpd_register_uri_handler(server, &logs);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

void init_webserver(void)
{
    ESP_LOGI(TAG, "Starting HTTP Server");
    static httpd_handle_t server = NULL;
    server = start_webserver();
}
