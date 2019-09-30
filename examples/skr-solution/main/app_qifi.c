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
#include "esp_http_client.h"
#include "sensor.h"

#include "app_camera.h"
#include "app_qifi.h"
#include "qifi_parser.h"
#include "quirc.h"

#define APP_QIFI_PRINT_QR 1
#define APP_QIFI_CONNECT_TIMEOUT_MS     30000   // ms

typedef enum {
    SKR_STATE_MIN = 0,
    SKR_QR_SCANNING,
    SKR_QIFI_STRING_PARSE_OK,
    SKR_QIFI_STRING_PARSE_FAIL,
    SKR_WIFI_CONNECTING,
    SKR_WIFI_CONNECTED,
    SKR_WIFI_CONNECT_TIMEOUT,
    SKR_WIFI_GOT_IP,
    SKR_POST_CAPTURE,
    SKR_STATE_MAX,
} skr_state_t;

static skr_state_t s_skr_state;
static qifi_parser_t parser;
static esp_timer_handle_t app_wifi_connect_timer;
static esp_http_client_handle_t http_client;

static char *TAG = "app-qifi";

static void app_qifi_set_skr_state(skr_state_t state)
{
    s_skr_state = state;
}

static skr_state_t app_qifi_get_skr_state(void)
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
        app_qifi_set_skr_state(SKR_WIFI_CONNECTED);
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        app_qifi_set_skr_state(SKR_WIFI_GOT_IP);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        break;

    default:
        break;
    }
    return ESP_OK;
}

void app_initialise_wifi(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA));
}

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

static void app_qifi_connect_wifi(qifi_parser_t* parser)
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

static const char *app_data_type_str(int dt)
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

static void app_dump_cells(const struct quirc_code *code)
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

static void app_dump_data(const struct quirc_data *data)
{
    printf("    Version: %d\n", data->version);
    printf("    ECC level: %c\n", "MLHQ"[data->ecc_level]);
    printf("    Mask: %d\n", data->mask);
    printf("    Data type: %d (%s)\n", data->data_type,
           app_data_type_str(data->data_type));
    printf("    Length: %d\n", data->payload_len);
    printf("\033[31m    Payload: %s\n", data->payload);

    qifi_parser_init(&parser);

    if (qifi_parser_parse(&parser, (const char*)(data->payload), data->payload_len) == ESP_OK) {
        app_qifi_set_skr_state(SKR_QIFI_STRING_PARSE_OK);
    } else {
        app_qifi_set_skr_state(SKR_QIFI_STRING_PARSE_FAIL);
    }

    if (data->eci) {
        printf("\033[31m    ECI: %d\n", data->eci);
    }
    printf("\033[0m\n");
}

static void app_dump_info(struct quirc *q, uint8_t count)
{
    printf("%d QR-codes found:\n\n", count);
    for (int i = 0; i < count; i++) {
        struct quirc_code code;
        struct quirc_data data;

        // Extract the QR-code specified by the given index.
        quirc_extract(q, i, &code);

        //Decode a QR-code, returning the payload data.
        quirc_decode_error_t err = quirc_decode(&code, &data);

#if APP_QIFI_PRINT_QR
        app_dump_cells(&code);
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
            app_dump_data(&data);
        }
        printf("\n");
    }
}

static void app_wifi_connect_cb(void* arg)
{
    skr_state_t state = app_qifi_get_skr_state();

    if (state < SKR_WIFI_GOT_IP) {
        app_qifi_set_skr_state(SKR_WIFI_CONNECT_TIMEOUT);
    }
}

static esp_err_t app_qrcode_scan(struct quirc *qr_recognizer)
{
    int id_count = 0;
    camera_fb_t *fb = NULL;
    uint8_t *image = NULL;
    // Save image width and height, avoid allocate memory repeatly.
    static uint16_t old_width = 0;
    static uint16_t old_height = 0;

    // Capture a frame
    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return ESP_FAIL;
    }

    if (old_width != fb->width || old_height != fb->height) {
        ESP_LOGI(TAG, "Recognizer(%p) size change w h len: %d, %d, %d", qr_recognizer, fb->width, fb->height, fb->len);
        // Resize the QR-code recognizer.
        if (quirc_resize(qr_recognizer, fb->width, fb->height) < 0) {
            ESP_LOGE(TAG, "Resize the QR-code recognizer err.");
            return ESP_FAIL;
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
        return ESP_FAIL;
    }

    // Print information of QR-code
    app_dump_info(qr_recognizer, id_count);
    esp_camera_fb_return(fb);
    return ESP_OK;
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

static void app_init_http_config(void)
{
    esp_http_client_config_t config = {
        .url = "http://192.168.3.63:8070",
        .event_handler = _http_event_handler,
    };

    http_client = esp_http_client_init(&config);
}

static esp_err_t app_post_capture(void)
{
    camera_fb_t *fb = NULL;
    // Capture a frame
    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return ESP_FAIL;
    }
    printf("captured: %p(%d) w:%d h:%d format:%d\n", fb->buf, fb->len, fb->width, fb->height, fb->format);

    // POST
    esp_http_client_set_method(http_client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(http_client, (const char*)(fb->buf), fb->len);
    esp_err_t err = esp_http_client_perform(http_client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(http_client),
                esp_http_client_get_content_length(http_client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    
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
            app_qifi_connect_wifi(&parser);
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

    app_init_http_config();

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
            app_qifi_set_skr_state(SKR_POST_CAPTURE);
            break;

        case SKR_POST_CAPTURE:
            app_post_capture();
            vTaskDelay(3000 / portTICK_RATE_MS);
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

    xTaskCreate(app_qifi_task, "app_qifi_task", 1024 * 50, NULL, 5, NULL);
}

void skr_start_app_capture_task(void)
{
    // Initialize camera
    if (app_camera_init(PIXFORMAT_JPEG) != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed, ready to restart..");
        esp_restart();
    }

    xTaskCreate(app_capture_task, "app_capture_task", 1024 * 50, NULL, 5, NULL);
}