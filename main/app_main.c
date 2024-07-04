
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "touch.h"
#include "audio.h"
#include "board.h"
#include "network.h"
#include "logger.h"
#include "myspiffs.h"
#include "i2c_bus.h"

#define TAG "main"

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(spiffs_init());
    i2c_bus_init();
    touch_init();
    audio_init();
    init_logger();
    init_network();
}
