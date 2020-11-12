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
#include "sensor.h"

#include "app_camera.h"
#include "qifi_parser.h"
#include "quirc.h"
#include "app_qrcode_task.h"
#include "app_qrcode_info.h"

volatile int QR_Code_Parse_OK_Flag = 0;

static QR_code_state_t QR_state;
static TaskHandle_t QR_code_info_task_Handle = NULL;

static char *TAG = "app-qrcode_task";

static QR_code_state_t app_get_QR_code_state(void)
{
    return QR_state;
}

void app_set_QR_code_state(QR_code_state_t state)
{
    QR_state = state;
}

void QR_code_info_task(void *parameter)
{
    QR_code_state_t QRstate;
    struct quirc *qr_recognizer = NULL;
    camera_config_t *camera_config = app_get_camera_cfg();

    if (!camera_config) {
        ESP_LOGI("QR_code", "err");
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
        QRstate = app_get_QR_code_state();

        // ESP_LOGI("QR_code", "%d", QRstate);

        switch (QRstate) {
        case QR_STATE_MIN:
            app_set_QR_code_state(QR_SCANNING);
            break;

        case QR_SCANNING:
            QR_code_info_scan(qr_recognizer);
            break;

        case QR_QIFI_STRING_PARSE_FAIL:
            app_set_QR_code_state(QR_SCANNING);
            break;

        case QR_QIFI_STRING_PARSE_OK:
            QR_Code_Parse_OK_Flag = 1;
            break;

        default:
            break;
        }

        if(QRstate == QR_QIFI_STRING_PARSE_OK){
            app_set_QR_code_state(QR_SCANNING);
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void identify_QR_code_info_task(void)
{
    BaseType_t xReturn = pdPASS;
    xReturn = xTaskCreate(QR_code_info_task, "QR_code_info_task", QR_code_info_task_stack*2, NULL, 5, &QR_code_info_task_Handle);
    if(xReturn == pdPASS)
        printf("QR_code_info_task Creat success!\n\n");
}


