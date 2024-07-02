#include "mqttclient.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "logger.h"
#include "cJSON.h"

static const char *TAG = "MQTT_EXAMPLE";

esp_mqtt_client_handle_t client = NULL;
static char subscribed_topic[256]; // Buffer to store the subscribed topic

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

        // Parse the JSON data
        cJSON *root = cJSON_Parse(event->data);
        if (root == NULL)
        {
            ESP_LOGE(TAG, "Failed to parse JSON");
            break;
        }

        // Extract event and song_id
        cJSON *event_item = cJSON_GetObjectItem(root, "event");
        cJSON *song_id_item = cJSON_GetObjectItem(root, "song_id");

        if (!cJSON_IsNumber(event_item) || !cJSON_IsNumber(song_id_item))
        {
            ESP_LOGE(TAG, "Invalid JSON format");
            cJSON_Delete(root);
            break;
        }

        // Check if event_item is a valid EventType
        EventType event_type = (EventType)event_item->valueint;
        if (event_type < PLAY || event_type > STOP)
        {
            ESP_LOGE(TAG, "Unknown event type");
            cJSON_Delete(root);
            break;
        }

        // Log the buffer entry
        ESP_LOGI(TAG, "Received event: %s, song ID: %d", getEventName(event_type), song_id_item->valueint);

        // Write to the buffer
        buffer_write(event_type, song_id_item->valueint);

        cJSON_Delete(root);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_err_t mqtt_app_start(const char *broker, const char *topic)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    strncpy(subscribed_topic, topic, sizeof(subscribed_topic) - 1);
    subscribed_topic[sizeof(subscribed_topic) - 1] = '\0';

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "Connecting to broker: %s", broker);
    ESP_LOGI(TAG, "Subscribing to topic: %s", topic);

    return ESP_OK;
}
