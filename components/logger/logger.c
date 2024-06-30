#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "logger.h"

#define TAG "logger"
#define NVS_NAMESPACE "storage"

circular_buffer_t buffer;

void erase_namespace()
{
    nvs_handle_t logger;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
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

uint8_t buffer_read()
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

    uint8_t value = buffer.data[buffer.tail];
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
    ESP_LOGI(TAG, "Read %d from buffer", value);
    return value;
}

void buffer_print()
{
    ESP_LOGI(TAG, "Buffer content:");
    for (int i = 0; i < buffer.count; i++)
    {
        int index = (buffer.tail + i) % BUFFER_SIZE;
        ESP_LOGI(TAG, "%d: %d", i, buffer.data[index]);
    }
}

void buffer_write(uint8_t value)
{
    nvs_handle_t logger;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &logger);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    buffer.data[buffer.head] = value;
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
    ESP_LOGI(TAG, "Written %d to buffer", value);
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

void init_logger(void)
{
    buffer_init();

    buffer_write(0);
    buffer_write(1);
    buffer_write(2);
    //     buffer_write(3);
    //     buffer_write(4);
    //     buffer_write(5);
    //     buffer_write(6);
    //     buffer_write(7);
    //     buffer_write(8);
    //     buffer_write(9);
    //     buffer_write(10);
    //     buffer_write(11);
    //     buffer_write(12);
    //     buffer_write(13);
    //     buffer_write(14);
    //     buffer_write(15);
    //     buffer_write(16);
    //     buffer_write(17);
    //     buffer_write(18);
    //     buffer_write(19);

    //     buffer_print();

    //     uint8_t value = buffer_read();
    //     ESP_LOGI(TAG, "Read value 1: %d", value);

    //     buffer_write(0);
    //     buffer_write(1);
    //     buffer_write(2);
    //     buffer_write(3);
    //     buffer_write(4);
    //     buffer_write(5);
    //     buffer_write(6);
    //     buffer_write(7);
    //     buffer_write(8);
    //     buffer_write(9);
    //     buffer_write(10);

    //     buffer_print();

    //     buffer_write(10);
    //     buffer_write(11);
    //     buffer_write(12);
    //     buffer_write(13);
    //     buffer_write(14);

    //     value = buffer_read();
    //     ESP_LOGI(TAG, "Read value 2: %d", value);
}
