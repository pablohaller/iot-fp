#include "logger.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "player"

void player_task(void *pvParameters)
{
    while (1)
    {
        buffer_entry_t last_entry;
        if (get_last_entry(&last_entry))
        {
            ESP_LOGI(TAG, "Healthcheck - Last Event: %s, Song ID: %d, Timestamp: %lld",
                     getEventName(last_entry.event), last_entry.song_id, (long long)last_entry.timestamp);
        }
        else
        {
            ESP_LOGI(TAG, "Healthcheck - Buffer is empty");
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
    }
}

void init_player()
{
    xTaskCreate(player_task, "player_task", 4096, NULL, 5, NULL);
}
