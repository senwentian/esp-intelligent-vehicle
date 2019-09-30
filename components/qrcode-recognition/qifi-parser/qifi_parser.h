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

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#define QIFI_STRING_MAX             128
#define QIFI_STRING_MIN             16

#define QIFI_SSID_LEN               32
#define QIFI_PASSWORD_LEN           64

#define ESP_ERR_QIFI_BASE           0x16000  /*!< Starting number of QiFi error codes */
#define ESP_ERR_QIFI_INVLALID_ARGS  (ESP_ERR_QIFI_BASE + 1)
#define ESP_ERR_QIFI_NO_SCHEME      (ESP_ERR_QIFI_BASE + 2)
#define ESP_ERR_QIFI_NO_AUTH_TYPE   (ESP_ERR_QIFI_BASE + 3)
#define ESP_ERR_QIFI_NO_SSID        (ESP_ERR_QIFI_BASE + 4)
#define ESP_ERR_QIFI_NO_PASSWORD    (ESP_ERR_QIFI_BASE + 5)
#define ESP_ERR_QIFI_NO_HIDDEN      (ESP_ERR_QIFI_BASE + 6)
#define ESP_ERR_QIFI_PARSE_FAILED   (ESP_ERR_QIFI_BASE + 7)

typedef enum {
    QIFI_WEP,
    QIFI_WPA,
    QIFI_NOPASS,
    QIFI_OMIT,
} auth_type_t;

/*
--------------------------------------------------------------------------------------------------------
Wi-Fi Network config (Android, iOS 11+)
We propose a syntax like "MECARD" for specifying wi-fi configuration.
Scanning such a code would, after prompting the user, configure the device's Wi-Fi accordingly. Example:

WIFI:T:WPA;S:mynetwork;P:mypass;;

--------------------------------------------------------------------------------------------------------
Parameter       Example         Description
--------------------------------------------------------------------------------------------------------
    T           WPA         Authentication type; can be WEP or WPA, or 'nopass' for no password. Or, omit for no password.
    S           mynetwork   Network SSID. Required.
                            Enclose in double quotes if it is an ASCII name, but could be interpreted as hex (i.e. "ABCD")
    P           mypass      Password, ignored if T is "nopass" (in which case it may be omitted).
                            Enclose in double quotes if it is an ASCII name, but could be interpreted as hex (i.e. "ABCD")
    H           true        Optional. True if the network SSID is hidden.
--------------------------------------------------------------------------------------------------------

Order of fields does not matter. Special characters \, ;, , and : should be escaped with a backslash (\) as in MECARD encoding.
For example, if an SSID was literally "foo;bar\baz" (with double quotes part of the SSID name itself)
then it would be encoded like: WIFI:S:\"foo\;bar\\baz\";;
more details see as: https://github.com/zxing/zxing/wiki/Barcode-Contents#wi-fi-network-config-android-ios-11
*/

typedef struct {
    auth_type_t type: 8;
    bool ssid_hidden;
    uint8_t ssid[QIFI_SSID_LEN];
    uint8_t ssid_len;
    uint8_t password[QIFI_PASSWORD_LEN];
    uint8_t password_len;
} qifi_parser_t;

/**
 * @brief       Init QiFi parser
 *
 * @param[in]   parser: a configuration to init
 *
 * @noreturn
*/
void qifi_parser_init(qifi_parser_t *parser);

/**
 * @brief       Parse QRCode of WiFi string
 *
 * @param[in]   buf: WiFi string
 * @param[in]   buflen: WiFi string length
 * @param[out]  parser: parse result
 *
 * @return
 *          - ESP_OK: if parse OK
 *          - others: see as ESP_ERR_QIFI_
*/
esp_err_t qifi_parser_parse(qifi_parser_t *parser, const char *buf, size_t buflen);


#ifdef __cplusplus
}
#endif
