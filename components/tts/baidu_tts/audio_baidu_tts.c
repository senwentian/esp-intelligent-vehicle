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
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_client.h"

#include "nvs_flash.h"
#include "jsmn.h"

#include "mp3_decoder.h"
#include "audio_baidu_tts.h"

#define TRUNKED_TOKEN_MESSAGE_LEN_MAX       1024
#define API_TTS_URL                         "http://tsn.baidu.com/text2audio"
#define API_TOKEN_URL                       "http://openapi.baidu.com/oauth/2.0/token"
#define MAX_TOKEN_SIZE                      100
#define BAIDU_TTS_CHAR_MAX_LEN              (512 * 3)
#define BAIDU_TTS_URL_MAX_LEN               (512 * 3 + 256)   // 512*3: 512 Chinese characters, 256: http header item
#define MP3_DECODER_FIFO_LEN_MIN            2048

typedef enum {
    S_MIN = 0,
    S_TTS_ERROR,
    S_TTS_TOKEN_IN_PROCESS,
    S_TTS_TOKEN_DONE,
    S_TTS_SPEECH_IN_PROCESS,
    S_TTS_SPEECH_DONE,
} baidu_tts_state_t;

typedef struct {
    char token[MAX_TOKEN_SIZE];
    char cuid[20];
    int spd;
    int pit;
    int vol;
    int per;
    int aue;
    char format[4];
} tts_config_t;

typedef esp_err_t (*baidu_tts_speech_cb_t)(baidu_tts_state_t state, uint8_t* data, uint32_t len);
static char *p_trunked_token_message;
static baidu_tts_state_t s_tts_state;
static uint32_t token_msg_len;
static tts_config_t* sp_tts_config;
static baidu_tts_speech_cb_t s_baidu_tts_speech_cb;

static const char *TAG = "http";

static void set_tts_state(baidu_tts_state_t state)
{
    s_tts_state = state;
}

static baidu_tts_state_t get_tts_state(void)
{
    return s_tts_state;
}

static esp_err_t user_tts_cb(baidu_tts_state_t state, uint8_t* data, uint32_t len)
{
    ESP_LOGI(TAG, "state:%d, recv len:%u, heap:%u", state, len, esp_get_free_heap_size());
    audio_ringbuff_t* audio_ringbuf_handle = mp3_decoder_get_ringbuffer_handle();
    if (audio_ringbuf_handle != NULL) {
        xRingbufferSend(audio_ringbuf_handle->ring_buf, (void*)data, len, portMAX_DELAY);
        audio_ringbuf_handle->filled_len += len;
        if (audio_ringbuf_handle->filled_len > MP3_DECODER_FIFO_LEN_MIN) {
            esp_start_mp3_decoder_task();
        }
    } else {
        ESP_LOGE(TAG, "Not init ringbuffer");
    }

    if(state == 5) {
        ESP_LOGI(TAG, "receive TTS done");
    }

    return ESP_OK;
}

static esp_err_t _tts_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            set_tts_state(S_TTS_ERROR);
            if (get_tts_state() == S_TTS_SPEECH_IN_PROCESS && s_baidu_tts_speech_cb) {
                s_baidu_tts_speech_cb(S_TTS_ERROR, evt->data, evt->data_len);
            }
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
            if (get_tts_state() == S_TTS_TOKEN_IN_PROCESS) {
                memcpy(p_trunked_token_message + token_msg_len, evt->data, evt->data_len);
                token_msg_len += evt->data_len;
            }

            if (get_tts_state() == S_TTS_SPEECH_IN_PROCESS && s_baidu_tts_speech_cb) {
                s_baidu_tts_speech_cb(S_TTS_SPEECH_IN_PROCESS, evt->data, evt->data_len);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");

            if (get_tts_state() == S_TTS_TOKEN_IN_PROCESS) {
                set_tts_state(S_TTS_TOKEN_DONE);
            }

            if (get_tts_state() == S_TTS_SPEECH_IN_PROCESS) {
                set_tts_state(S_TTS_SPEECH_DONE);
                if (s_baidu_tts_speech_cb) {
                    s_baidu_tts_speech_cb(S_TTS_SPEECH_DONE, evt->data, evt->data_len);
                }
            }

            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            set_tts_state(S_TTS_ERROR);
            if (get_tts_state() == S_TTS_SPEECH_IN_PROCESS && s_baidu_tts_speech_cb) {
                s_baidu_tts_speech_cb(S_TTS_ERROR, evt->data, evt->data_len);
            }
            break;
    }

    return ESP_OK;
}

static bool jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return true;
    }
    return false;
}

static void parse_token_from_string(const char *json_string, const char *token_key, char *token_value)
{
    jsmn_parser parser;
    jsmn_init(&parser);
    jsmntok_t t[20];
    int i;

    int r = jsmn_parse(&parser, json_string, strlen(json_string), t, 20);

    if (r < 0) {
        ESP_LOGE(TAG, "Failed to parse JSON: %d", r);
        return NULL;
    }
    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        ESP_LOGE(TAG, "Object expected");
        return NULL;
    }

    for (i = 1; i < r; i++) {
        if (jsoneq(json_string, &t[i], token_key) && i < r) {
            int tok_len = t[i+1].end - t[i+1].start;
            memcpy(token_value, json_string + t[i+1].start, tok_len);
        }
    }
    return NULL;
}

static esp_err_t http_get_speech_resource(char* text, tts_config_t* config, baidu_tts_speech_cb_t cb)
{
    if (text == NULL || config == NULL) {
        return ESP_FAIL;
    }
    if (cb) {
        s_baidu_tts_speech_cb = cb;
    }

    char* post_data = (char *)calloc(1, BAIDU_TTS_URL_MAX_LEN);
    char params_pattern[] = "ctp=1&lan=zh&cuid=%s&tok=%s&tex=%s&per=%d&spd=%d&pit=%d&vol=%d&aue=%d";
    int32_t data_len = snprintf(post_data, BAIDU_TTS_URL_MAX_LEN, params_pattern, config->cuid, config->token, text,
             config->per, config->spd, config->pit, config->vol, config->aue);

    ESP_LOGI(TAG, "TTS URL:%s?%.*s", API_TTS_URL, data_len, post_data);

    esp_http_client_config_t httpconfig = {
        .url = API_TTS_URL,
        .event_handler = _tts_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&httpconfig);

    set_tts_state(S_TTS_SPEECH_IN_PROCESS);

    esp_http_client_set_post_field(client, post_data, data_len);
    esp_http_client_set_method(client, HTTP_METHOD_POST);

    // POST
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    free(post_data);
    post_data = NULL;
    esp_http_client_cleanup(client);
    return ESP_OK;
}

static void http_get_baidu_tts_token(char *token)
{
    char url_pattern[] = "%s?grant_type=client_credentials&client_id=%s&client_secret=%s";
    char url[200] = {0};
    snprintf(url, 200, url_pattern, API_TOKEN_URL, CONFIG_API_KEY, CONFIG_SECRET_KEY);
    esp_http_client_config_t httpconfig = {
        .url = url,
        .event_handler = _tts_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&httpconfig);

    if (!p_trunked_token_message) {
        p_trunked_token_message = (char *)calloc(1, TRUNKED_TOKEN_MESSAGE_LEN_MAX);
    }
    
    set_tts_state(S_TTS_TOKEN_IN_PROCESS);

    // GET
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d", esp_http_client_get_status_code(client));
        // parse token
        parse_token_from_string(p_trunked_token_message, "access_token", token);
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    free(p_trunked_token_message);
    p_trunked_token_message = NULL;
    esp_http_client_cleanup(client);
}

static void baidu_tts_init(void)
{
    // Ring Buffer Init
   mp3_ringbuffer_init();

    if (!sp_tts_config) {
        sp_tts_config = (tts_config_t *)calloc(1, sizeof(tts_config_t));
    }

    http_get_baidu_tts_token(sp_tts_config->token);
    ESP_LOGI(TAG, "TTS init done");
}

static void baidu_tts_deinit(void)
{
    if (sp_tts_config) {
        free(sp_tts_config);
        sp_tts_config = NULL;
    }
    token_msg_len = 0;

    ESP_LOGI(TAG, "TTS deinit done");
}

static tts_config_t* baidu_get_tts_default_config(void)
{
    if (!sp_tts_config) {
        return NULL;
    }

    int per = 0;    // {0, 1, 2, 3, 4, 5, 103, 106, 110, 111}, speech from different people
    int spd = 5;    // [0, 9], speech speed
    int pit = 5;    // [0, 9], speech tone
    int vol = 5;    // [0, 9], speech volume
    int aue = 3;    // {3.mp3, 4.pcm-16k, 5.pcm-8k, 6.wav} // speech format
    
    snprintf(sp_tts_config->cuid, sizeof(sp_tts_config->cuid), "1234567C");
    sp_tts_config->per = per;
    sp_tts_config->spd = spd;
    sp_tts_config->pit = pit;
    sp_tts_config->vol = vol;
    sp_tts_config->aue = aue;
    
    const char formats[4][4] = {"mp3", "pcm", "pcm", "wav"};
    snprintf(sp_tts_config->format, sizeof(sp_tts_config->format), formats[aue - 3]);
    return sp_tts_config;
}

esp_err_t tts_download(char* tts_name)
{
    esp_err_t ret = ESP_OK;

    baidu_tts_init();

    tts_config_t* config = baidu_get_tts_default_config();

    if (config) {
        ret = http_get_speech_resource(tts_name, config, user_tts_cb);
    }

    baidu_tts_deinit();
    return ret;
}
