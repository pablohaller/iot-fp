#include "logger.h"
#include "cJSON.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_sntp.h"
#include <string.h>
#include <time.h>
#include "audio.h"

#define TAG "logger"
#define NVS_NAMESPACE "storage"

circular_buffer_t buffer;
SemaphoreHandle_t buffer_semaphore;

void ntp_sync_time(void)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // Esperar a que el tiempo se sincronice
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

char *buffer_to_json()
{
    xSemaphoreTake(buffer_semaphore, portMAX_DELAY);

    cJSON *root = cJSON_CreateObject();
    cJSON *buffer_array = cJSON_CreateArray();

    for (int i = 0; i < buffer.count; i++)
    {
        int index = (buffer.tail + i) % BUFFER_SIZE;
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "event", cJSON_CreateString(getEventName(buffer.data[index].event)));
        cJSON_AddItemToObject(entry, "song_id", cJSON_CreateNumber(buffer.data[index].song_id));

        if (buffer.data[index].timestamp != 0)
        {
            char timestamp_str[20];
            struct tm *timeinfo = localtime((time_t *)&buffer.data[index].timestamp);
            strftime(timestamp_str, sizeof(timestamp_str), "%m/%d/%Y %H:%M:%S", timeinfo);
            cJSON_AddItemToObject(entry, "timestamp", cJSON_CreateString(timestamp_str));
        }
        else
        {
            cJSON_AddItemToObject(entry, "timestamp", cJSON_CreateNull());
        }

        cJSON_AddItemToArray(buffer_array, entry);
    }

    cJSON_AddItemToObject(root, "buffer", buffer_array);
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);

    xSemaphoreGive(buffer_semaphore);
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
    xSemaphoreTake(buffer_semaphore, portMAX_DELAY);

    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        xSemaphoreGive(buffer_semaphore);
        return 0;
    }

    if (buffer.count == 0)
    {
        ESP_LOGI(TAG, "Buffer is empty");
        nvs_close(logger);
        xSemaphoreGive(buffer_semaphore);
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

    xSemaphoreGive(buffer_semaphore);
    return 1;
}

void buffer_print()
{
    xSemaphoreTake(buffer_semaphore, portMAX_DELAY);

    ESP_LOGI(TAG, "Buffer content:");
    for (int i = 0; i < buffer.count; i++)
    {
        int index = (buffer.tail + i) % BUFFER_SIZE;
        ESP_LOGI(TAG, "%d: Event: %s, Song ID: %d", i, getEventName(buffer.data[index].event), buffer.data[index].song_id);
    }

    xSemaphoreGive(buffer_semaphore);
}

void buffer_write(EventType event, uint8_t song_id)
{
    xSemaphoreTake(buffer_semaphore, portMAX_DELAY);

    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        xSemaphoreGive(buffer_semaphore);
        return;
    }

    time_t now;
    time(&now);

    buffer.data[buffer.head].event = event;
    buffer.data[buffer.head].song_id = song_id;
    buffer.data[buffer.head].timestamp = now;
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
    ESP_LOGI(TAG, "Written event: %s, song ID: %d, timestamp: %lld to buffer", getEventName(event), song_id, (long long)now);

    xSemaphoreGive(buffer_semaphore);
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

circular_buffer_t get_buffer_from_nvs()
{
    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }

    circular_buffer_t local_buffer;
    size_t required_size = sizeof(circular_buffer_t);
    err = nvs_get_blob(logger, "buffer", &local_buffer, &required_size);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "Buffer not found, initializing a new one");
        memset(&local_buffer, 0, sizeof(circular_buffer_t));
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
    return local_buffer;
}

uint8_t get_last_entry(buffer_entry_t *entry)
{
    xSemaphoreTake(buffer_semaphore, portMAX_DELAY);

    if (buffer.count == 0)
    {
        xSemaphoreGive(buffer_semaphore);
        return 0;
    }

    *entry = buffer.data[(buffer.head - 1 + BUFFER_SIZE) % BUFFER_SIZE];

    xSemaphoreGive(buffer_semaphore);
    return 1;
}


const char *EventNames[] = {
    "PLAY/PAUSE",
    "NEXT",
    "PREVIOUS",
    "STOP",
    "VOLUME_UP",
    "VOLUME_DOWN"};

const char *getEventName(EventType event)
{
    return EventNames[event];
}

void init_logger(void)
{
    buffer_semaphore = xSemaphoreCreateMutex();
    buffer_init();


    char *json_string = buffer_to_json();
    ESP_LOGI(TAG, "Buffer JSON: %s", json_string);

    free(json_string);
}
