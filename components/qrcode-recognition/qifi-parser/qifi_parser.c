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
#include <string.h>
#include <stdbool.h>
#include "esp_err.h"
#include "qifi_parser.h"
#include "esp_log.h"

#ifndef LOWER
#define LOWER(c)                (unsigned char)(c | 0x20)
#endif

#ifndef IS_ALPHA
#define IS_ALPHA(c)             (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#endif

#define QIFI_SCHEME_STRING      "WIFI:"

#define QIFI_AUTH_WEP           "T:WEP;"
#define QIFI_AUTH_WPA           "T:WPA;"
#define QIFI_AUTH_NOPASS        "T:nopass;"
#define QIFI_AUTH_OMIT          "T:omit;"
#define QIFI_AUTH_LEN_MAX       strlen(QIFI_AUTH_NOPASS)

typedef enum {
    QIFI_SCHEME = 0,
    QIFI_AUTH_TYPE,
    QIFI_SSID,
    QIFI_PASSWORD,
    QIFI_HIDDEN,
} qifi_field_t;

typedef enum {
    qs_init = 0,
    qs_dead,
    qs_scheme_start,
    qs_new_field_start,
} qifi_state_t;

static const char* TAG = "qifi";

void qifi_parser_init(qifi_parser_t* parser)
{
    if (parser) {
        memset(parser, 0x0, sizeof(qifi_parser_t));
    }

    return;
}

static esp_err_t qifi_parser_result(qifi_state_t state, qifi_field_t field)
{
    if (state == qs_dead) {
        return ESP_ERR_QIFI_PARSE_FAILED;
    }

    // QiFi parse result check
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

static const char* qifi_parser_state_qs_init(const char *p, const char* bufend, qifi_field_t* field, qifi_state_t* state)
{
    if (IS_ALPHA(*p)) {
        *state = qs_scheme_start;
        if ((p + strlen(QIFI_SCHEME_STRING) < bufend)
            && (strncmp(p, QIFI_SCHEME_STRING, strlen(QIFI_SCHEME_STRING)) == 0)) {
                *field |= (1 << QIFI_SCHEME);
                *state = qs_new_field_start;
                p += strlen(QIFI_SCHEME_STRING) - 1;
        } else {
            *state = qs_dead;
        }
    }

    return p;
}

static const char* qifi_parser_state_qs_new_field(const char* p, const char* bufend, qifi_field_t* field, qifi_state_t* state, qifi_parser_t* parser)
{
    switch (*p) {
    case 'T':   // T:nopass
        if (p + QIFI_AUTH_LEN_MAX > bufend) {
            *state = qs_dead;
            break;
        }

        if (strncmp(p, QIFI_AUTH_WEP, strlen(QIFI_AUTH_WEP)) == 0) {
            *field |= (1 << QIFI_AUTH_TYPE);
            *state = qs_new_field_start;
            p += strlen(QIFI_AUTH_WEP) - 1;
            parser->type = QIFI_WEP;
        } else if (strncmp(p, QIFI_AUTH_WPA, strlen(QIFI_AUTH_WPA)) == 0) {
            *field |= (1 << QIFI_AUTH_TYPE);
            *state = qs_new_field_start;
            p += strlen(QIFI_AUTH_WPA) - 1;
            parser->type = QIFI_WPA;
        } else if (strncmp(p, QIFI_AUTH_NOPASS, strlen(QIFI_AUTH_NOPASS)) == 0) {
            *field |= (1 << QIFI_AUTH_TYPE);
            *state = qs_new_field_start;
            p += strlen(QIFI_AUTH_NOPASS) - 1;
            parser->type = QIFI_NOPASS;
        } else if (strncmp(p, QIFI_AUTH_OMIT, strlen(QIFI_AUTH_OMIT)) == 0) {
            *field |= (1 << QIFI_AUTH_TYPE);
            *state = qs_new_field_start;
            p += strlen(QIFI_AUTH_OMIT) - 1;
            parser->type = QIFI_OMIT;
        } else {
            *state = qs_dead;
            break;
        }
        break;

    case 'S':   // S:<ssid>;
        

    case 'P':   // P:<password>;
    
    case 'H':   // H:true;

    case ';':   // H:false;

    default:
        state = qs_dead;
        break;
    }

    return p;
}

esp_err_t qifi_parser_parse(const char *buf, size_t buflen, qifi_parser_t* parser)
{
    qifi_field_t field = 0x0;
    const char *p = NULL;
    qifi_state_t state = qs_init;
    const char* bufend = buf + buflen;

    if ((buf == NULL) || (buflen > QIFI_STRING_MAX) || (buflen < QIFI_STRING_MIN) || (parser == NULL)) {
        return ESP_ERR_QIFI_INVLALID_ARGS;
    }

    // QiFi parse start
    for (p = buf; p < bufend; p++) {
        switch (state) {
        case qs_init:
            p = qifi_parser_state_qs_init(p, bufend, &field, &state);
            break;

        case qs_scheme_start:
            state = qs_dead;
            break;

        case qs_new_field_start:
            p = qifi_parser_state_qs_new_field(p, bufend, &field, &state, parser);
            break;

        default:
            state = qs_dead;
            break;
        }

        if (state == qs_dead) {
            break;
        }
    }

    return qifi_parser_result(state, field);
}
