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
#include <stdlib.h>
#include "qifi_parser.h"
#include "app_qrcode_wifi_task.h"
#include "quirc.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "app_camera.h"
#include "app_qrcode_wifi.h"

#define APP_QIFI_PRINT_QR               CONFIG_APP_QIFI_PRINT_QR

static qifi_parser_t parser;

static const char *TAG = "app-qrcode";

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

qifi_parser_t *app_get_qifi_parser(void)
{
    return &parser;
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

    if (qifi_parser_parse(&parser, (const char *)(data->payload), data->payload_len) == ESP_OK) {
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

esp_err_t app_qrcode_scan(struct quirc *qr_recognizer)
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


