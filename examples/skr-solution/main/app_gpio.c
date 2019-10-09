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
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/timers.h"
#include "app_gpio.h"
#include "esp_wifi.h"

#define APP_PRESS_GPIO     0
#define APP_PRESS_GPIO_INPUT_PIN_SEL  (1ULL << APP_PRESS_GPIO)
#define ANTI_SHAKE_TIME     100 // ms

static xQueueHandle gpio_evt_queue;
static TimerHandle_t  s_app_press_factory_timer;
static uint32_t s_app_old_time;

static const char *TAG = "gpio";

static void gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void app_press_factory_button(void *param)
{
    if (gpio_get_level(APP_PRESS_GPIO) == 0) {
        ESP_LOGI(TAG, "Restore the Factory Settings..");

        ESP_ERROR_CHECK(esp_wifi_restore());

        printf("ready to restart..\r\n");
        esp_restart();
        printf("restarted?\n");
    }
}

static void app_gpio_task(void *arg)
{
    uint32_t io_num = 0;

    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));

            uint32_t app_current_time = clock() * (1000 / CLOCKS_PER_SEC);

            if (io_num == APP_PRESS_GPIO) {
                if (gpio_get_level(APP_PRESS_GPIO) == 0) {
                    if ((app_current_time - s_app_old_time) > ANTI_SHAKE_TIME) {
                        xTimerStart(s_app_press_factory_timer, portMAX_DELAY);
                        s_app_old_time = app_current_time;
                    }
                } else {
                    xTimerStop(s_app_press_factory_timer, portMAX_DELAY);
                }
            }
        }
    } // end for
}

void app_gpio_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = APP_PRESS_GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_set_intr_type(APP_PRESS_GPIO, GPIO_INTR_ANYEDGE);

    gpio_evt_queue = xQueueCreate(5, sizeof(uint32_t));
    xTaskCreate(app_gpio_task, "app_gpio_task", 2048, NULL, 10, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(APP_PRESS_GPIO, gpio_isr_handler, (void *) APP_PRESS_GPIO);

    s_app_press_factory_timer = xTimerCreate("restore-tmr", (3000 / portTICK_RATE_MS), pdTRUE, NULL, app_press_factory_button);
}
