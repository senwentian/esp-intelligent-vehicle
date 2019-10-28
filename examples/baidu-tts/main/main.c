// Copyright 2019-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

#include "ES8388_interface.h"

#include "app_wifi.h"

#include "mp3_decoder.h"
#include "audio_baidu_tts.h"

#define RING_BUFFER_SIZE 8*1024

audio_ringbuff_t audio_ringbuf;
static const char *TAG = "TTS_MAIN";

// Set ES8388
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

void app_main()
{
    int ret = 0;
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    audio_ringbuf.ring_buf = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
    audio_ringbuf.filled_len = 0;

    app_wifi_initialise();
    app_wifi_wait_connected();
    ESP_LOGI(TAG, "Connected to AP, begin audio example");

    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
        .intr_alloc_flags = 0,                                                  //Default interrupt priority
        .use_apll = 1                                             //Auto clear tx descriptor on underflow
    };

    i2s_driver_install(0, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .bck_io_num = 5,
        .ws_io_num = 25,
        .data_out_num = 26,
        .data_in_num = 35                                       
    };

    i2s_set_pin(0, &pin_config);
#define GPIO_PA_EN 21
printf("init 8388\r\n");
    ret |= Es8388Init(&initConf);
    ret |= Es8388ConfigFmt(ES_MODULE_ADC_DAC, ES_I2S_NORMAL);
    ret |= Es8388SetBitsPerSample(ES_MODULE_ADC_DAC, BIT_LENGTH_16BITS);
    if(ret != 0){
        printf("ES8388 init error\n");
    }
    Es8388Start(ES_MODULE_ADC_DAC);

    SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL<<GPIO_PA_EN));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 1);

    Es8388SetVoiceVolume(40);

    char* tts = "支付宝到帐一百万元";

    tts_download(tts);

}