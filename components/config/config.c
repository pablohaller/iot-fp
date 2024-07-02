#include <stdio.h>
#include "config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "cJSON.h"

#define TAG "config"
#define CONFIG_NAMESPACE "config"

esp_err_t config_set_value(const char *key, const char *value)
{
    nvs_handle_t config_handle;
    esp_err_t err;

    // Open NVS namespace
    err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &config_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    // Set the key-value pair
    err = nvs_set_str(config_handle, key, value);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) setting value for key %s!", esp_err_to_name(err), key);
        nvs_close(config_handle);
        return err;
    }

    // Commit the changes
    err = nvs_commit(config_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing changes!", esp_err_to_name(err));
        nvs_close(config_handle);
        return err;
    }

    nvs_close(config_handle);
    ESP_LOGI(TAG, "Successfully set value for key %s", key);
    return ESP_OK;
}

char *config_get_all_as_json(void)
{
    nvs_handle_t config_handle;
    esp_err_t err;
    char *json_string = NULL;
    size_t required_size;
    char *value;

    // Open NVS namespace
    err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &config_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }

    // Create JSON root object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to create JSON root object");
        nvs_close(config_handle);
        return NULL;
    }

    // Iterate over all keys in the namespace
    nvs_iterator_t it = nvs_entry_find(NVS_DEFAULT_PART_NAME, CONFIG_NAMESPACE, NVS_TYPE_STR);
    while (it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        // Get the value size
        err = nvs_get_str(config_handle, info.key, NULL, &required_size);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) getting size for key %s", esp_err_to_name(err), info.key);
            cJSON_Delete(root);
            nvs_close(config_handle);
            return NULL;
        }

        // Allocate memory for the value
        value = malloc(required_size);
        if (value == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for value");
            cJSON_Delete(root);
            nvs_close(config_handle);
            return NULL;
        }

        // Get the value
        err = nvs_get_str(config_handle, info.key, value, &required_size);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error (%s) getting value for key %s", esp_err_to_name(err), info.key);
            free(value);
            cJSON_Delete(root);
            nvs_close(config_handle);
            return NULL;
        }

        // Add the key-value pair to the JSON object
        cJSON_AddStringToObject(root, info.key, value);

        // Free the allocated memory
        free(value);

        // Move to the next entry
        it = nvs_entry_next(it);
    }

    // Convert JSON object to string
    json_string = cJSON_Print(root);
    if (json_string == NULL)
    {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        nvs_close(config_handle);
        return NULL;
    }

    // Cleanup
    cJSON_Delete(root);
    nvs_close(config_handle);

    return json_string;
}
