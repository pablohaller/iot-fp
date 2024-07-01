#include "logger.h"
#include "cJSON.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define TAG "logger"
#define NVS_NAMESPACE "storage"

circular_buffer_t buffer;

char *buffer_to_json()
{
    cJSON *root = cJSON_CreateObject();
    cJSON *buffer_array = cJSON_CreateArray();

    for (int i = 0; i < buffer.count; i++)
    {
        int index = (buffer.tail + i) % BUFFER_SIZE;
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "event", cJSON_CreateString(getEventName(buffer.data[index].event)));
        cJSON_AddItemToObject(entry, "song_id", cJSON_CreateNumber(buffer.data[index].song_id));
        cJSON_AddItemToArray(buffer_array, entry);
    }

    cJSON_AddItemToObject(root, "buffer", buffer_array);
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}

void erase_namespace()
{
    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    nvs_iterator_t it = nvs_entry_find(NULL, NVS_NAMESPACE, NVS_TYPE_ANY);
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        ESP_LOGI(TAG, "Erasing key: %s", info.key);
        nvs_erase_key(logger, info.key);
        it = nvs_entry_next(it);
    }

    err = nvs_commit(logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing changes!", esp_err_to_name(err));
    }

    nvs_close(logger);
    ESP_LOGI(TAG, "Namespace '%s' erased.", NVS_NAMESPACE);
}

uint8_t buffer_read(buffer_entry_t *entry)
{
    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return 0;
    }

    if (buffer.count == 0)
    {
        ESP_LOGI(TAG, "Buffer is empty");
        nvs_close(logger);
        return 0;
    }

    *entry = buffer.data[buffer.tail];
    buffer.tail = (buffer.tail + 1) % BUFFER_SIZE;
    buffer.count--;

    err = nvs_set_blob(logger, "buffer", &buffer, sizeof(circular_buffer_t));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) updating buffer in NVS", esp_err_to_name(err));
    }
    else
    {
        nvs_commit(logger);
    }

    nvs_close(logger);
    ESP_LOGI(TAG, "Read event: %s, song ID: %d", getEventName(entry->event), entry->song_id);
    return 1;
}

void buffer_print()
{
    ESP_LOGI(TAG, "Buffer content:");
    for (int i = 0; i < buffer.count; i++)
    {
        int index = (buffer.tail + i) % BUFFER_SIZE;
        ESP_LOGI(TAG, "%d: Event: %s, Song ID: %d", i, getEventName(buffer.data[index].event), buffer.data[index].song_id);
    }
}

void buffer_write(EventType event, uint8_t song_id)
{
    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    buffer.data[buffer.head].event = event;
    buffer.data[buffer.head].song_id = song_id;
    buffer.head = (buffer.head + 1) % BUFFER_SIZE;

    if (buffer.count == BUFFER_SIZE)
    {
        buffer.tail = (buffer.tail + 1) % BUFFER_SIZE;
    }
    else
    {
        buffer.count++;
    }

    err = nvs_set_blob(logger, "buffer", &buffer, sizeof(circular_buffer_t));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) writing buffer to NVS", esp_err_to_name(err));
    }
    else
    {
        nvs_commit(logger);
    }

    nvs_close(logger);
    ESP_LOGI(TAG, "Written event: %s, song ID: %d to buffer", getEventName(event), song_id);
}

void buffer_init()
{
    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    size_t required_size = sizeof(circular_buffer_t);
    err = nvs_get_blob(logger, "buffer", &buffer, &required_size);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "Buffer not found, initializing a new one");
        memset(&buffer, 0, sizeof(circular_buffer_t));
        nvs_set_blob(logger, "buffer", &buffer, sizeof(circular_buffer_t));
        nvs_commit(logger);
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading buffer from NVS");
    }
    else
    {
        ESP_LOGI(TAG, "Buffer loaded from NVS");
    }

    nvs_close(logger);
}

const char *EventNames[] = {
    "Play",
    "Pause",
    "Next",
    "Previous",
    "Stop"};

const char *getEventName(EventType event)
{
    return EventNames[event];
}

void init_logger(void)
{
    buffer_init();

    buffer_write(PLAY, 1);
    buffer_write(PAUSE, 2);
    buffer_write(NEXT, 3);

    char *json_string = buffer_to_json();
    ESP_LOGI(TAG, "Buffer JSON: %s", json_string);

    free(json_string);
}
