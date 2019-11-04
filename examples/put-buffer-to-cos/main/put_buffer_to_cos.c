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
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "crypto/base64.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "mbedtls/sha1.h"
#include "crypto/sha1.h"
#include "rom/md5_hash.h"

#define FILENAME_RAW_LEN_MAX            32
#define FILENAME_ENCODE_LEN_MAX         64
#define COS_PPC_BUFFER_LEN_MAX          256
static time_t s_ppctime;

static const char *TAG = "ppc";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
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

void esp_calc_md5sum(void *buffer, uint32_t len, uint8_t md5value[16])
{
    struct MD5Context context;
    MD5Init(&context);
    MD5Update(&context, buffer, len);
    MD5Final(md5value, &context);
    return;
}

int esp_http_url_encode(const char* raw_url, const int raw_url_len, char* out_url, int max_url_len)
{
    int i = 0, j = 0;
    char ch;
 
    if ((raw_url == NULL) || (out_url == NULL) || (raw_url_len <= 0) || (max_url_len <= 0)) {
        return -1;
    }

    for (i = 0; (i < raw_url_len) && (j < max_url_len); ++i) {
        ch = raw_url[i];
        if (((ch >= 'A') && (ch <= 'Z')) ||
            ((ch >= 'a') && (ch <= 'z')) ||
            ((ch >= '0') && (ch <= '9'))) {
            out_url[j++] = ch;
        } else if (ch == ' ') {
            out_url[j++] = '+';
        } else if (ch == '.' || ch == '-' || ch == '_' || ch == '*') {
            out_url[j++] = ch;
        } else if (j + 3 < max_url_len) {
            sprintf(out_url + j, "%%%02X", (unsigned char)ch);
            j += 3;
        } else {
            return -2;
        }
    }

    out_url[j] = '\0';
    return j;
}

static void app_set_ppc_time(void)
{
    time(&s_ppctime);
}

static time_t app_get_ppc_time(void)
{
    return s_ppctime;
}

int app_get_filename_by_time(char* ppcbuffer, int max_ppcbuffer_len)
{
    time_t now = app_get_ppc_time();
    struct tm timeinfo = {0};

    if (ppcbuffer == NULL) {
        return -1;
    }

    localtime_r(&now, &timeinfo);
    return snprintf(ppcbuffer, max_ppcbuffer_len, "%04d/M%02d/D%02d/T%02d%02d%02d.jpg",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
        timeinfo.tm_mday, timeinfo.tm_hour,
        timeinfo.tm_min, timeinfo.tm_sec);
}

int app_get_date_string(char* ppcbuffer, int max_ppcbuffer_len)
{
    time_t now = app_get_ppc_time();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    return strftime(ppcbuffer, max_ppcbuffer_len, "%a, %d %b %Y %X GMT", &timeinfo);
}

int app_get_url_string(char *ppcbuffer, int max_ppcbuffer_len)
{
    char filename[FILENAME_RAW_LEN_MAX] = {0};
    app_get_filename_by_time(filename, FILENAME_RAW_LEN_MAX);

    char filename_urlencode[FILENAME_ENCODE_LEN_MAX] = {0};
    esp_http_url_encode(filename, strlen(filename), filename_urlencode, FILENAME_ENCODE_LEN_MAX);
    return snprintf(ppcbuffer, max_ppcbuffer_len, "http://%s-%s.cos.%s.myqcloud.com/%s",
            CONFIG_TECENT_COS_BUCKET_NAME, CONFIG_TENCENT_COS_APPID,
            CONFIG_TECENT_COS_BUCKET_AREA, filename_urlencode);
}

int app_get_md5_string(void* buffer, uint32_t buffer_len, char* ppcbuffer, int max_ppcbuffer_len)
{
    uint8_t md5value[16] = {0};
    esp_calc_md5sum(buffer, buffer_len, md5value);

    size_t base64_md5_len = 0;
    unsigned char* base64_md5 = base64_encode(md5value, 16, &base64_md5_len);
    snprintf(ppcbuffer, max_ppcbuffer_len, "%s", base64_md5);
    free(base64_md5);
    return base64_md5_len;
}

int app_get_auth_string(char* ppcbuffer, int max_ppcbuffer_len)
{
    int i = 0;
    int ret_len = 0;
    char signtime[32] = {0};
    unsigned char sharet[20];
    unsigned char sha1rets[41] = {0};
    unsigned char hmacsha1rets[41] = {0};
    char http_put[128] = {0};

    char filename[FILENAME_RAW_LEN_MAX] = {0};
    app_get_filename_by_time(filename, FILENAME_RAW_LEN_MAX);

    // sha1
    ret_len = snprintf(ppcbuffer, max_ppcbuffer_len, "put\n/%s\n\nhost=%s-%s.cos.%s.myqcloud.com\n",
        filename, CONFIG_TECENT_COS_BUCKET_NAME, CONFIG_TENCENT_COS_APPID, CONFIG_TECENT_COS_BUCKET_AREA);

    mbedtls_sha1_ret((const unsigned char *)ppcbuffer, ret_len, sharet);
    for (i = 0; i < 20; ++i) {
        snprintf((char *)(sha1rets + 2*i), 3, "%02x", sharet[i]);
    }

    // hmac-sha1 for signtime
    ret_len = snprintf(signtime, 32, "%ld;%ld", app_get_ppc_time(), app_get_ppc_time() + CONFIG_PPC_EXPIRE_S);
    hmac_sha1((const uint8_t *)CONFIG_TECENT_COS_ACCESS_SECRET_KEY,
        strlen(CONFIG_TECENT_COS_ACCESS_SECRET_KEY), (const uint8_t *)signtime, ret_len, sharet);
    for (i = 0; i < 20; ++i) {
        snprintf((char *)(hmacsha1rets + 2*i), 3, "%02x", sharet[i]);
    }

    // hmac-sha1 for http request
    ret_len = snprintf(http_put, 128, "sha1\n%s\n%s\n", signtime, sha1rets);
    hmac_sha1((const uint8_t *)hmacsha1rets, 40, (const uint8_t *)http_put, ret_len, sharet);
    for (i = 0; i < 20; ++i) {
        snprintf((char *)(hmacsha1rets + 2*i), 3, "%02x", sharet[i]);
    }

    return snprintf(ppcbuffer, max_ppcbuffer_len, 
        "q-sign-algorithm=sha1&q-ak=%s&q-sign-time=%s&q-key-time=%s&q-header-list=host&q-url-param-list=&q-signature=%s",
        CONFIG_TECENT_COS_ACCESS_SECRET_ID, signtime, signtime, hmacsha1rets);
}

static void app_http_put_picture(void* buffer, uint32_t buffer_len)
{
    app_set_ppc_time();

    char* ppcbuffer = (char *)calloc(1, COS_PPC_BUFFER_LEN_MAX);
    app_get_url_string(ppcbuffer, COS_PPC_BUFFER_LEN_MAX);

    esp_http_client_config_t config = {
        .url = ppcbuffer,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // PUT
    esp_http_client_set_method(client, HTTP_METHOD_PUT);

    snprintf(ppcbuffer, COS_PPC_BUFFER_LEN_MAX, "%s-%s.cos.%s.myqcloud.com",
        CONFIG_TECENT_COS_BUCKET_NAME, CONFIG_TENCENT_COS_APPID, CONFIG_TECENT_COS_BUCKET_AREA);
    esp_http_client_set_header(client, "Host", ppcbuffer);
    esp_http_client_set_header(client, "User-Agent", "cos-sdk-c/5.0.5(Compatible Unknown)");
    esp_http_client_set_header(client, "Accept", "*/*");

    snprintf(ppcbuffer, COS_PPC_BUFFER_LEN_MAX, "%d", buffer_len);
    esp_http_client_set_header(client, "Content-Length", ppcbuffer);
    esp_http_client_set_header(client, "Content-Type", "text/plain");

    app_get_md5_string(buffer, buffer_len, ppcbuffer, COS_PPC_BUFFER_LEN_MAX);
    esp_http_client_set_header(client, "Content-MD5", ppcbuffer);

    esp_http_client_set_header(client, "x-cos-security-token", "MyTokenString");

    app_get_date_string(ppcbuffer, COS_PPC_BUFFER_LEN_MAX);
    esp_http_client_set_header(client, "Date", ppcbuffer);

    app_get_auth_string(ppcbuffer, COS_PPC_BUFFER_LEN_MAX);
    esp_http_client_set_header(client, "Authorization", ppcbuffer);

    esp_http_client_set_post_field(client, buffer, buffer_len);

    // PUT now
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP PUT Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(ppcbuffer);
    ppcbuffer = NULL;
}

void app_put_picture_to_cos(void* buffer, uint32_t len)
{
    // init
    // ...
    // TODO: Check and Create bucket if not exist. Bucket named <APPNAME>-<MAC address>, such as esp-skr-240ac4045afc
    // more chores

    app_http_put_picture(buffer, len);

    // deinit
    // ...
}

