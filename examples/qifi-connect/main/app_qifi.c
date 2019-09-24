/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "app_camera.h"
#include "qifi_parser.h"
#include "quirc.h"

#define PRINT_QR 1

typedef enum {
    SKR_STATE_MIN = 0,
    SKR_SCANNING,
    SKR_QIFI_STRING_PARSE_OK,
    SKR_QIFI_STRING_PARSE_FAIL,
    SKR_WIFI_CONNECTING,
    SKR_WIFI_CONNECTED,
    SKR_WIFI_CONNECT_TIMEOUT,
    SKR_WIFI_GOT_IP,
    SKR_STATE_MAX,
} skr_state_t;

static skr_state_t s_skr_state;
static qifi_parser_t parser;

static char *TAG = "app-qifi";

void qifi_set_skr_state(skr_state_t state)
{
    s_skr_state = state;
}

skr_state_t qifi_get_skr_state(void)
{
    return s_skr_state;
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    
    case SYSTEM_EVENT_STA_CONNECTED:
        qifi_set_skr_state(SKR_WIFI_CONNECTED);
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        qifi_set_skr_state(SKR_WIFI_GOT_IP);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        break;

    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA));
}

static void qifi_connect_wifi(qifi_parser_t* parser)
{
    wifi_config_t wifi_config;
    memset(&wifi_config, 0x0, sizeof(wifi_config_t));
    strncpy((char*)(wifi_config.sta.ssid), (char*)(parser->ssid), sizeof(wifi_config.sta.ssid));

    if (parser->type < QIFI_NOPASS) {
        strncpy((char*)(wifi_config.sta.password), (char*)(parser->password), sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static const char *data_type_str(int dt)
{
    switch (dt) {
    case QUIRC_DATA_TYPE_NUMERIC:
        return "NUMERIC";
    case QUIRC_DATA_TYPE_ALPHA:
        return "ALPHA";
    case QUIRC_DATA_TYPE_BYTE:
        return "BYTE";
    case QUIRC_DATA_TYPE_KANJI:
        return "KANJI";
    }
    return "unknown";
}

static void dump_cells(const struct quirc_code *code)
{
    int u = 0, v = 0;

    printf("    %d cells, corners:", code->size);
    for (u = 0; u < 4; u++) {
        printf(" (%d,%d)", code->corners[u].x, code->corners[u].y);
    }
    printf("\n");

    for (v = 0; v < code->size; v++) {
		printf("\033[0m    ");
        for (u = 0; u < code->size; u++) {
            int p = v * code->size + u;

            if (code->cell_bitmap[p >> 3] & (1 << (p & 7))) {
				printf("\033[40m  ");
            } else {
				printf("\033[47m  ");
            }
        }
		printf("\033[0m\n");
    }
}

static void dump_data(const struct quirc_data *data)
{
    printf("    Version: %d\n", data->version);
    printf("    ECC level: %c\n", "MLHQ"[data->ecc_level]);
    printf("    Mask: %d\n", data->mask);
    printf("    Data type: %d (%s)\n", data->data_type,
           data_type_str(data->data_type));
    printf("    Length: %d\n", data->payload_len);
    printf("\033[31m    Payload: %s\n", data->payload);

    qifi_parser_init(&parser);

    if (qifi_parser_parse((const char*)(data->payload), data->payload_len, &parser) == ESP_OK) {
        qifi_set_skr_state(SKR_QIFI_STRING_PARSE_OK);
    } else {
        qifi_set_skr_state(SKR_QIFI_STRING_PARSE_FAIL);
    }

    if (data->eci) {
        printf("\033[31m    ECI: %d\n", data->eci);
    }
    printf("\033[0m\n");
}

static void dump_info(struct quirc *q, uint8_t count)
{
    printf("%d QR-codes found:\n\n", count);
    for (int i = 0; i < count; i++) {
        struct quirc_code code;
        struct quirc_data data;

        // Extract the QR-code specified by the given index.
        quirc_extract(q, i, &code);

        //Decode a QR-code, returning the payload data.
        quirc_decode_error_t err = quirc_decode(&code, &data);

#if PRINT_QR
        dump_cells(&code);
        printf("\n");
#endif

        if (err) {
            printf("  Decoding FAILED: %s\n", quirc_strerror(err));
        } else {
            printf("  Decoding successful:\n");
            printf("    %d cells, corners:", code.size);
            for (uint8_t u = 0; u < 4; u++) {
                printf(" (%d,%d)", code.corners[u].x, code.corners[u].y);
            }
            printf("\n");
            dump_data(&data);
        }
        printf("\n");
    }
}

void qifi_task(void *parameter)
{
    struct quirc *qr_recognizer = NULL;
    camera_fb_t *fb = NULL;
    uint8_t *image = NULL;
    int id_count = 0;
    skr_state_t state;
    // Save image width and height, avoid allocate memory repeatly.
    uint16_t old_width = 0;
    uint16_t old_height = 0;

    initialise_wifi();

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
    }

    while (1) {
        state = qifi_get_skr_state();

        if (state == SKR_WIFI_GOT_IP) {
            break;
        }

        switch (state) {
        case SKR_STATE_MIN:
            qifi_set_skr_state(SKR_SCANNING);
            break;

        case SKR_SCANNING:
            // Capture a frame
            fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE(TAG, "Camera capture failed");
                continue;
            }

            if (old_width != fb->width || old_height != fb->height) {
                ESP_LOGD(TAG, "Recognizer size change w h len: %d, %d, %d", fb->width, fb->height, fb->len);
                ESP_LOGI(TAG, "Resize the QR-code recognizer.");
                // Resize the QR-code recognizer.
                if (quirc_resize(qr_recognizer, fb->width, fb->height) < 0) {
                    ESP_LOGE(TAG, "Resize the QR-code recognizer err.");
                    continue;
                } else {
                    old_width = fb->width;
                    old_height = fb->height;
                }
            }

            image = quirc_begin(qr_recognizer, NULL, NULL);
            memcpy(image, fb->buf, fb->len);
            quirc_end(qr_recognizer);

            // Return the number of QR-codes identified in the last processed image.
            id_count = quirc_count(qr_recognizer);
            if (id_count == 0) {
                ESP_LOGW(TAG, "invalid WiFi QR code");
                esp_camera_fb_return(fb);
                continue;
            }

            // Print information of QR-code
            dump_info(qr_recognizer, id_count);
            esp_camera_fb_return(fb);
            break;

        case SKR_QIFI_STRING_PARSE_FAIL:
            qifi_set_skr_state(SKR_SCANNING);
            break;

        case SKR_QIFI_STRING_PARSE_OK:
            qifi_connect_wifi(&parser);
            qifi_set_skr_state(SKR_WIFI_CONNECTING);
            break;

        case SKR_WIFI_CONNECTING:
            // 
            break;

        case SKR_WIFI_CONNECTED:
            break;

        case SKR_WIFI_CONNECT_TIMEOUT:
            qifi_set_skr_state(SKR_SCANNING);
            break;

        case SKR_WIFI_GOT_IP:
            break;

        default:
            break;
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }

    // Destroy QR-Code recognizer (quirc)
    quirc_destroy(qr_recognizer);
    vTaskDelete(NULL);
}

void start_qifi_task(void)
{
    xTaskCreate(qifi_task, "qifi_task", 1024 * 40, NULL, 5, NULL);
}

