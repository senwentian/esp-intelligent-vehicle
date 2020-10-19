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
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"

#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_camera.h"

#include "user_camera.h"
#include "user_qrcode_wifi_task.h"
#include "user_wifi.h"
#include "user_qrcode_task.h"
#include "user_qrcode_info.h"
#include "user_httpd.h"
#include "user_mdns.h"

#include "user_init.h"
#include "user_motor.h"
#include "user_remote.h"
#include "user_uart.h"
#include "user_sbus.h"
#include "user_control.h"
#include "user_TCP_Rev_Parse.h"
#include "user_pwm.h"
#include "user_ota.h"
#include "user_TCP.h"
#include "user_send_current.h"

static TaskHandle_t Control_Task_Handle = NULL;
static TaskHandle_t UART_Recieve_Task_Handle = NULL;
static TaskHandle_t Test_Task_Handle = NULL;
static TaskHandle_t Send_Val_Task_Handle = NULL;
static TaskHandle_t TCP_Server_Task_Handle = NULL;
static TaskHandle_t QR_Code_Info_Task_Handle = NULL;
static TaskHandle_t Parse_Send_Task_Handle = NULL;
static TaskHandle_t PC_Control_Task_Handle = NULL;
static TaskHandle_t CAN_Recieve_Task_Handle = NULL;
static TaskHandle_t Slow_frequency_Task_Handle = NULL;
static TaskHandle_t Fast_frequency_Task_Handle = NULL;

static void Creat_Task(void *arg);
static void Control_Task(void *arg);
static void UART_Recieve_Task(void *arg);
static void Test_Task(void * arg);
static void Sent_Val_Task(void * arg);
static void TCP_Server_Task(void * arg);
static void QR_Code_Info_Task(void * arg);
static void Parse_Send_Task(void * arg);
static void PC_Control_Task(void * arg);
static void CAN_Recieve_Task(void *arg);
static void OTA_Task(void * arg);
static void Slow_frequency_Task(void * arg);
static void Fast_frequency_Task(void * arg);

static const char *TAG = "main";
char Task_Buffer[700] = {0};

#define TEST_TASK 0

void app_main()
{
    wifi_config_t wifi_config;

    // Initialize flash
    esp_err_t ret = nvs_flash_init();

    led_Init();
    xTaskCreate((TaskFunction_t)Slow_frequency_Task, "Slow_frequency_Task", 1024, NULL, 6, &Slow_frequency_Task_Handle);

    button_Init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    user_wifi_init();

    user_camera_init();

    ret = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    if (ret == ESP_OK && wifi_config.sta.ssid[0] != 0) {
        ESP_LOGI("main", "Connect to SSID:%s Password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        skr_start_wifi_connect(&wifi_config);
        wait_for_ip();
    } else {
        ESP_LOGI("main", "Start QiFi netconfig");
        skr_start_app_qifi_task();
    }

    button_Init();

    xTaskCreate((TaskFunction_t)OTA_Task, "OTA_Task", 1024, NULL, 5, NULL);
    vTaskDelete(Slow_frequency_Task_Handle);
    xTaskCreate((TaskFunction_t)Fast_frequency_Task, "Fast_frequency_Task", 1024, NULL, 6, &Fast_frequency_Task_Handle);

#if CONFIG_VIDEO_SYNC
    user_httpd_main();
    user_mdns_main();
    printf("open the Video sync succeed\n");
#endif

    TCP_Server_Init();

    vTaskDelete(Fast_frequency_Task_Handle);

    user_Init();

    BaseType_t xReturn = pdPASS;
    xReturn = xTaskCreate((TaskFunction_t)Creat_Task, "Creat_Task", 2048, NULL, 6, NULL);
    if(xReturn == pdPASS){
        printf("Creat Task is success!\n\n\n"); 
    }
}

static void Creat_Task(void *arg)
{
    BaseType_t xReturn = pdPASS;

#if CONFIG_PC_CONTROL_OPERATION_MODE
    xReturn = xTaskCreate((TaskFunction_t)PC_Control_Task, "PC_Control_Task", 1024, NULL, 5, &PC_Control_Task_Handle);
    if(xReturn == pdPASS)
        printf("PC_Control_Task Creat success!\n\n");
#else
    xReturn = xTaskCreate((TaskFunction_t)Control_Task, "Control_Task", 1024*2, NULL, 5, &Control_Task_Handle);
    if(xReturn == pdPASS)
        printf("Control_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)UART_Recieve_Task, "UART_Recieve_Task", 1024, NULL, 5, &UART_Recieve_Task_Handle);
    if(xReturn == pdPASS)
        printf("UART_Recieve_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)Sent_Val_Task, "Sent_Val_Task", 1024, NULL, 5, &Send_Val_Task_Handle);
    if(xReturn == pdPASS)
        printf("Sent_Val_Task Creat success!\n\n");
#endif

#if TEST_TASK
    xReturn = xTaskCreate((TaskFunction_t)Test_Task, "Test_Task", 1024*2, NULL, 5, &Test_Task_Handle);
    if(xReturn == pdPASS)
        printf("Test_Task Creat success!\n\n");
#endif

    xReturn = xTaskCreate((TaskFunction_t)CAN_Recieve_Task, "CAN_Recieve_Task", 1024*3, NULL, 6, &CAN_Recieve_Task_Handle);
    if(xReturn == pdPASS)
        printf("CAN_Recieve_Task Creat success!\n\n");


    xReturn = xTaskCreate((TaskFunction_t)TCP_Server_Task, "TCP_Server_Task", 1024, NULL, 5, &TCP_Server_Task_Handle);
    if(xReturn == pdPASS)
        printf("TCP_Server_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)Parse_Send_Task, "Parse_Send_Task", 1024, NULL, 5, &Parse_Send_Task_Handle);
    if(xReturn == pdPASS)
        printf("Parse_Send_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)QR_Code_Info_Task, "QR_Code_Info_Task", 1024, NULL, 5, &QR_Code_Info_Task_Handle);
    if(xReturn == pdPASS)
        printf("QR_Code_Info_Task Creat success!\n\n");

    vTaskDelete(NULL);
}

static void Test_Task(void *arg)
{
    int flag = 0;
    while (1) {
        if(flag == 0){
            vTaskGetRunTimeStats(Task_Buffer);
            ESP_LOGI("***************Task_State_List***************", "\n\nName            Total Time      Percentage \n%s", Task_Buffer);
        }else{
           vTaskList(Task_Buffer);
           ESP_LOGI("***************Task_State_List***************", "\n\nName          State  Priority Stackleft Number CoreID\n%s", Task_Buffer);
        }
        printf("\nbuffer size:%d\n", strlen(Task_Buffer));
        flag = !flag;
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

static void TCP_Server_Task(void * arg)
{
    TCP_Server_Task_Create();
    vTaskDelete(NULL);
}

static void Parse_Send_Task(void * arg)
{
    Parse_Send_Task_Create();
    vTaskDelete(NULL);
}

static void Sent_Val_Task(void *arg)
{
    Send_Current_Create();
    vTaskDelete(NULL);
}

static void Control_Task(void *arg)
{
    mcpwm_control();
    while(1)
    {
        UnderpanControlTask();
        vTaskDelay(20 / portTICK_RATE_MS);
    }
}

static void UART_Recieve_Task(void *arg)
{
    UART_Recieve_Task_Create();
    vTaskDelete(NULL);
}

static void QR_Code_Info_Task(void * arg)
{
    identify_QR_code_info_task();
    vTaskDelete(NULL);
}

static void PC_Control_Task(void * arg)
{
    PC_Remote_Control_Task();
    vTaskDelete(NULL);
}

static void CAN_Recieve_Task(void *arg)
{
    while(1)
    { 
        can_message_t RxMsg;
        can_receive(&RxMsg, portMAX_DELAY);
        C610_GetMotorInfo(&RxMsg);
    }
}

static void OTA_Task(void * arg)
{
    user_ota_task();
    vTaskDelete(NULL);
}

static void Fast_frequency_Task(void * arg)
{
    int Fast = 0;
    while(1){
        gpio_set_level(GPIO_NUM_2, Fast);
        Fast = !Fast;
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

static void Slow_frequency_Task(void * arg)
{
    int Slow = 0;
    while(1){
        gpio_set_level(GPIO_NUM_2, Slow);
        Slow = !Slow;
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}
