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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "app_wifi.h"

#include "ES8388_interface.h"
#include "mp3_decoder.h"
#include "audio_baidu_tts.h"

#define GPIO_PA_EN          21

static const char *TAG = "main";

void app_audio_driver_init(void)
{
    int ret = 0;

    // I2S init
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                  // Only TX
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,           // 2-channels
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
        .intr_alloc_flags = 0,                                  // Default interrupt priority
        .use_apll = 1                                           // Auto clear tx descriptor on underflow
    };

    i2s_driver_install(0, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .bck_io_num = 5,
        .ws_io_num = 25,
        .data_out_num = 26,
        .data_in_num = 35                                       
    };

    i2s_set_pin(0, &pin_config);

    // Codec init
    ESP_LOGI(TAG, "Init 8388");
    Es8388Config initConf = {
        .esMode = ES_MODE_SLAVE,
        .i2c_port_num = I2C_NUM_0,
        .i2c_cfg = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = 18,
            .scl_io_num = 23,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = 100000
        },
        .adcInput = ADC_INPUT_LINPUT1_RINPUT1,
        .dacOutput = DAC_OUTPUT_LOUT1 | DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT1 | DAC_OUTPUT_ROUT2,
    };

    ret |= Es8388Init(&initConf);
    ret |= Es8388ConfigFmt(ES_MODULE_ADC_DAC, ES_I2S_NORMAL);
    ret |= Es8388SetBitsPerSample(ES_MODULE_ADC_DAC, BIT_LENGTH_16BITS);

    if (ret != 0) {
        ESP_LOGE(TAG, "ES8388 init error");
    }

    Es8388Start(ES_MODULE_ADC_DAC);

    SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    gpio_config_t io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << GPIO_PA_EN));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 1);

    Es8388SetVoiceVolume(40);
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    app_audio_driver_init();

    app_wifi_initialise();

    app_wifi_wait_connected();
    ESP_LOGI(TAG, "Connected to AP, begin audio example");

    char* tts1 = "9 月 24 日，ESP-BLE-MESH 通过了蓝牙技术联盟（Bluetooth SIG）全功能支持的认证！";
    tts_download(tts1);

    char* tts2 = "我和我的祖国，一刻也不能分割.";
    tts_download(tts2);

    char* tts3 = "在那山的那边海的那边，有一群蓝精灵，他们活泼又聪明，他们调皮又灵敏，\
    他们自由自在生活在，那绿色的大森林，他们善良勇敢相互关心，\
    欧,可爱的蓝精灵，欧,可爱的蓝精灵，他们齐心协力开动脑筋，\
    斗败了格格巫，他们唱歌跳舞快乐又欢欣，mm qq，在那山的那边海的那边，有一群蓝精灵，他们活泼又聪明";
    tts_download(tts3);

    vTaskDelay(10000 / portTICK_RATE_MS);
    char* tts4 = "自古皆贵中华，贱夷狄，朕独爱之如一";
    tts_download(tts4);

    ESP_LOGI(TAG, "Test TTS End");
}
