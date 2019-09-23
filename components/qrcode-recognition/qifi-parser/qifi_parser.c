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
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "qifi_parser.h"

typedef enum {
    QIFI_SCHEME = 0,
    QIFI_AUTH_TYPE,
    QIFI_SSID,
    QIFI_PASSWORD,
    QIFI_HIDDEN,
} qifi_field_t;

void qifi_parser_init(qifi_parser_t* parser)
{
    if (parser) {
        memset(parser, 0x0, sizeof(qifi_parser_t));
    }

    return;
}

esp_err_t qifi_parser_parse(const char *buf, size_t buflen, qifi_parser_t* parser)
{
    qifi_field_t field = 0x0;
    if ((buf == NULL) || (buflen > QIFI_STRING_MAX) || (buflen < QIFI_STRING_MIN) || (parser == NULL)) {
        return ESP_ERR_QIFI_INVLALID_ARGS;
    }

    if (!(field & (1 << QIFI_SCHEME))) {
        return ESP_ERR_QIFI_NO_SCHEME;
    }

    if (!(field & (1 << QIFI_AUTH_TYPE))) {
        return ESP_ERR_QIFI_NO_AUTH_TYPE;
    }

    if (!(field & (1 << QIFI_SSID))) {
        return ESP_ERR_QIFI_NO_SSID;
    }

    if (!(field & (1 << QIFI_PASSWORD))) {
        return ESP_ERR_QIFI_NO_PASSWORD;
    }

    if (!(field & (1 << QIFI_HIDDEN))) {
        return ESP_ERR_QIFI_NO_HIDDEN;
    }

    return ESP_OK;
}