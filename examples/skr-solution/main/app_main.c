/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019-2020 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS chips only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_wifi.h"

#include "app_camera.h"
#include "app_qifi.h"
#include "app_gpio.h"
#include "esp_log.h"

static const char *TAG = "main";

static void skr_start_wifi_connect(wifi_config_t* config)
{
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void app_main()
{
    wifi_config_t wifi_config;

    // Initialize flash
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    app_gpio_init();

    app_initialise_wifi();

    ret = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    if (ret == ESP_OK && wifi_config.sta.ssid[0] != 0) {
        ESP_LOGI(TAG, "Connect to SSID:%s Password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        skr_start_wifi_connect(&wifi_config);
        skr_start_app_capture_task();
    } else {
        ESP_LOGI(TAG, "Start QiFi netconfig");
        skr_start_app_qifi_task();
    }
}
