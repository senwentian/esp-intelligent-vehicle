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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"

typedef enum {
    SKR_STATE_MIN = 0,
    SKR_QR_SCANNING,
    SKR_QIFI_STRING_PARSE_OK,
    SKR_QIFI_STRING_PARSE_FAIL,
    SKR_WIFI_CONNECTING,
    SKR_WIFI_CONNECTED,
    SKR_WIFI_CONNECT_TIMEOUT,
    SKR_WIFI_GOT_IP,
    SKR_SNTP_SYNC,
    SKR_SNTP_DONE,
    SKR_POST_CAPTURE,
    SKR_STATE_MAX,
} skr_state_t;

/**
 * @brief       Start QiFi task
*/
void skr_start_app_qifi_task(void);

/**
 * @brief       Start Capture task
*/
void skr_start_app_capture_task(void);

/**
 * @brief       Post capture
*/
esp_err_t app_post_capture(void);

/**
 * @brief       PUT picture to COS
*/
void app_put_picture_to_cos(void *buffer, uint32_t len);

/**
 * @brief       Set skr state
*/
void app_qifi_set_skr_state(skr_state_t state);
#ifdef __cplusplus
}
#endif
