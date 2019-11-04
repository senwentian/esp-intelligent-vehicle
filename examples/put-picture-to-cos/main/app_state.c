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
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "sensor.h"

#include "app_camera.h"
#include "app_wifi.h"
#include "qifi_parser.h"
#include "quirc.h"
#include "app_state.h"
#include "app_qrcode.h"

#define APP_QIFI_CONNECT_TIMEOUT_MS     CONFIG_APP_QIFI_CONNECT_TIMEOUT_MS

static skr_state_t s_skr_state;
static esp_timer_handle_t app_wifi_connect_timer;

static char *TAG = "app-state";

void app_qifi_set_skr_state(skr_state_t state)
{
    s_skr_state = state;
}

static skr_state_t app_qifi_get_skr_state(void)
{
    return s_skr_state;
}

static void app_qifi_connect_wifi(qifi_parser_t *parser)
{
    wifi_config_t wifi_config;
    memset(&wifi_config, 0x0, sizeof(wifi_config_t));
    strncpy((char *)(wifi_config.sta.ssid), (char *)(parser->ssid), sizeof(wifi_config.sta.ssid));

    if (parser->type < QIFI_NOPASS) {
        strncpy((char *)(wifi_config.sta.password), (char *)(parser->password), sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void app_wifi_connect_cb(void *arg)
{
    skr_state_t state = app_qifi_get_skr_state();

    if (state < SKR_WIFI_GOT_IP) {
        app_qifi_set_skr_state(SKR_WIFI_CONNECT_TIMEOUT);
    }
}

static void app_create_wifi_connect_timer(void)
{
    const esp_timer_create_args_t wifi_connect_timer_cfg = {
        .callback = &app_wifi_connect_cb,
        .name = "wifi-connect"
    };

    ESP_ERROR_CHECK(esp_timer_create(&wifi_connect_timer_cfg, &app_wifi_connect_timer));
}

static void app_start_wifi_connect_timer(void)
{
    ESP_ERROR_CHECK(esp_timer_start_once(app_wifi_connect_timer, (APP_QIFI_CONNECT_TIMEOUT_MS * 1000)));
}

static void app_stop_wifi_connect_timer(void)
{
    ESP_ERROR_CHECK(esp_timer_stop(app_wifi_connect_timer));
}

esp_err_t app_post_capture(void)
{
    // TODO: add mutex
    camera_fb_t *fb = NULL;
    // Capture a frame
    fb = esp_camera_fb_get();

    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return ESP_FAIL;
    }

    printf("captured: %p(%d) w:%d h:%d format:%d\n", fb->buf, fb->len, fb->width, fb->height, fb->format);

    if (app_qifi_get_skr_state() < SKR_SNTP_DONE) {
        return ESP_FAIL;
    }

    // PUT
    app_put_picture_to_cos(fb->buf, fb->len);
    return ESP_OK;
}


void app_qifi_task(void *parameter)
{
    skr_state_t state;
    struct quirc *qr_recognizer = NULL;
    camera_config_t *camera_config = app_get_camera_cfg();

    if (!camera_config) {
        vTaskDelete(NULL);
    }

    // Use QVGA Size currently, but quirc can support other frame size.(eg:
    // FRAMESIZE_QVGA,FRAMESIZE_HQVGA,FRAMESIZE_QCIF,FRAMESIZE_QQVGA2,FRAMESIZE_QQVGA,etc)
    if (camera_config->frame_size > FRAMESIZE_QVGA) {
        ESP_LOGE(TAG, "Camera Frame Size err %d, support maxsize is QVGA", (camera_config->frame_size));
        vTaskDelete(NULL);
    }

    // Construct a new QR-code recognizer.
    qr_recognizer = quirc_new();

    if (!qr_recognizer) {
        ESP_LOGE(TAG, "Can't create quirc object");
        vTaskDelete(NULL);
    }

    while (1) {
        state = app_qifi_get_skr_state();

        switch (state) {
        case SKR_STATE_MIN:
            app_qifi_set_skr_state(SKR_QR_SCANNING);
            break;

        case SKR_QR_SCANNING:
            app_qrcode_scan(qr_recognizer);
            break;

        case SKR_QIFI_STRING_PARSE_FAIL:
            app_qifi_set_skr_state(SKR_QR_SCANNING);
            break;

        case SKR_QIFI_STRING_PARSE_OK:
            app_qifi_connect_wifi(app_get_qifi_parser());
            app_qifi_set_skr_state(SKR_WIFI_CONNECTING);
            app_create_wifi_connect_timer();
            app_start_wifi_connect_timer();
            break;

        case SKR_WIFI_CONNECTING:
            break;

        case SKR_WIFI_CONNECTED:
            break;

        case SKR_WIFI_CONNECT_TIMEOUT:
            app_qifi_set_skr_state(SKR_QR_SCANNING);
            app_stop_wifi_connect_timer();
            break;

        case SKR_WIFI_GOT_IP:
            // Destroy QR-Code recognizer (quirc)
            quirc_destroy(qr_recognizer);
            break;

        default:
            break;
        }

        if (state == SKR_WIFI_GOT_IP) {
            break;
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }

    esp_camera_deinit();

    // esp_camera_init can not be initialzied without reboot
    // here reboot for a new esp_camera_init()
    ESP_LOGI(TAG, "ready to restart..");
    esp_restart();

    vTaskDelete(NULL);
}

void app_capture_task(void *parameter)
{
    skr_state_t state;

    while (1) {
        state = app_qifi_get_skr_state();

        switch (state) {
        case SKR_STATE_MIN:
        case SKR_QR_SCANNING:
        case SKR_QIFI_STRING_PARSE_FAIL:
        case SKR_QIFI_STRING_PARSE_OK:
        case SKR_WIFI_CONNECTING:
        case SKR_WIFI_CONNECTED:
            break;

        case SKR_WIFI_GOT_IP:
            // Destroy QR-Code recognizer (quirc)
            app_qifi_set_skr_state(SKR_SNTP_SYNC);
            break;

        case SKR_SNTP_SYNC:
            esp_wait_sntp_sync();
            break;

        case SKR_SNTP_DONE:
            app_qifi_set_skr_state(SKR_POST_CAPTURE);
            break;

        case SKR_POST_CAPTURE:
            app_post_capture();
            vTaskDelay(CONFIG_APP_POST_CAPTURE_INTERVAL_MS / portTICK_RATE_MS);
            break;

        default:
            break;
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }

    esp_camera_deinit();

    vTaskDelete(NULL);
}

void skr_start_app_qifi_task(void)
{
    // Initialize camera
    if (app_camera_init(PIXFORMAT_GRAYSCALE) != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed, ready to restart..");
        esp_restart();
    }

    xTaskCreate(app_qifi_task, "app_qifi_task", CONFIG_APP_QIFI_TASK_STACK, NULL, 5, NULL);
}

void skr_start_app_capture_task(void)
{
    // Initialize camera
    if (app_camera_init(PIXFORMAT_JPEG) != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed, ready to restart..");
        esp_restart();
    }

    xTaskCreate(app_capture_task, "app_capture_task", CONFIG_APP_CAPTURE_TASK_STACK, NULL, 5, NULL);
}
