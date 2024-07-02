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
#include "mqttclient.h"
#include "config.h"

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
        int status = wifi_init_sta(ssid_json->valuestring, password_json->valuestring);
        if (status == 200)
        {
            const char resp[] = "Connected to WiFi";
            httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        }
        else
        {
            const char resp[] = "Failed to connect to WiFi";
            httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        }
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
    return ESP_OK;
}

esp_err_t event_type_handler(httpd_req_t *req)
{
    char query[100];
    char event_str[10];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        ESP_LOGI(TAG, "Found URL query: %s", query);

        if (httpd_query_key_value(query, "event", event_str, sizeof(event_str)) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query parameter => event=%s", event_str);
        }
        else
        {
            ESP_LOGI(TAG, "No URL query parameter => event");
            const char resp[] = "Invalid query parameter: event";
            httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGI(TAG, "No URL query found");
        const char resp[] = "No query string found";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    EventType event = atoi(event_str);
    circular_buffer_t local_buffer = get_buffer_from_nvs();
    uint8_t last_song_id = (local_buffer.count > 0) ? local_buffer.data[(local_buffer.tail + local_buffer.count - 1) % BUFFER_SIZE].song_id : 0;

    buffer_write(event, last_song_id);

    ESP_LOGI(TAG, "Event: %s, Song ID: %d", getEventName(event), last_song_id);

    const char resp[] = "Event received and processed";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t mqtt_connect_handler(httpd_req_t *req)
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

    cJSON *json = cJSON_Parse(content);
    if (json == NULL)
    {
        const char resp[] = "Invalid JSON";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    const cJSON *broker_json = cJSON_GetObjectItemCaseSensitive(json, "broker");
    const cJSON *topic_json = cJSON_GetObjectItemCaseSensitive(json, "topic");

    if (cJSON_IsString(broker_json) && (broker_json->valuestring != NULL) &&
        cJSON_IsString(topic_json) && (topic_json->valuestring != NULL))
    {
        esp_err_t res = mqtt_app_start(broker_json->valuestring, topic_json->valuestring);
        cJSON_Delete(json);

        if (res == ESP_OK)
        {
            const char resp[] = "MQTT connected";
            httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        else
        {
            const char resp[] = "MQTT connection failed";
            httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }
    }
    else
    {
        cJSON_Delete(json);
        const char resp[] = "Invalid JSON fields";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    char *json_string = config_get_all_as_json();
    if (json_string == NULL)
    {
        const char *resp = "Error generating JSON";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);

    free(json_string);
    return ESP_OK;
}

static esp_err_t config_post_handler(httpd_req_t *req)
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

    const cJSON *key_json = cJSON_GetObjectItemCaseSensitive(json, "key");
    const cJSON *value_json = cJSON_GetObjectItemCaseSensitive(json, "value");

    if (cJSON_IsString(key_json) && (key_json->valuestring != NULL) &&
        cJSON_IsString(value_json) && (value_json->valuestring != NULL))
    {
        ESP_LOGI(TAG, "Parsed key: %s, value: %s", key_json->valuestring, value_json->valuestring);
        esp_err_t err = config_set_value(key_json->valuestring, value_json->valuestring);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set config value");
            cJSON_Delete(json);
            const char resp[] = "Failed to set config value";
            httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }
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

    const char resp[] = "Config set successfully";
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

static const httpd_uri_t event_type = {
    .uri = "/event-type",
    .method = HTTP_POST,
    .handler = event_type_handler};

static const httpd_uri_t logs = {
    .uri = "/logs",
    .method = HTTP_GET,
    .handler = logs_get_handler};

static const httpd_uri_t mqtt_connect = {
    .uri = "/mqtt-connect",
    .method = HTTP_POST,
    .handler = mqtt_connect_handler};

static const httpd_uri_t config_get_uri = {
    .uri = "/config",
    .method = HTTP_GET,
    .handler = config_get_handler};

static const httpd_uri_t config_post_uri = {
    .uri = "/config",
    .method = HTTP_POST,
    .handler = config_post_handler};

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
        httpd_register_uri_handler(server, &event_type);
        httpd_register_uri_handler(server, &mqtt_connect);
        httpd_register_uri_handler(server, &config_get_uri);
        httpd_register_uri_handler(server, &config_post_uri);
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
