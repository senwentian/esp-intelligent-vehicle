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
#include "driver/uart.h"
#include "driver/gpio.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_camera.h"

#include "app_camera.h"
#include "app_qrcode_wifi_task.h"
#include "app_wifi.h"
#include "app_gpio.h"
#include "app_qrcode_task.h"
#include "app_qrcode_info.h"


#include "user_init.h"
#include "user_motor.h"
#include "user_remote.h"
#include "user_uart.h"
#include "user_sbus.h"
#include "user_control.h"
#include "user_TCP_Rev_Parse.h"

#define PORT 3333
#define main_begin_wait_time 1
#define camera_stack 32768

static SemaphoreHandle_t main_begin;

static TaskHandle_t Creat_Task_Handle = NULL;
static TaskHandle_t Control_Task_Handle = NULL;
static TaskHandle_t UART_Recieve_Task_Handle = NULL;
static TaskHandle_t Test_Task_Handle = NULL;
static TaskHandle_t Send_Val_Task_Handle = NULL;
static TaskHandle_t TCP_Server_Task_Handle = NULL;
static TaskHandle_t QR_Code_Info_Task_Handle = NULL;
static TaskHandle_t Parse_Send_Task_Handle = NULL;
static TaskHandle_t PC_Control_Task_Handle = NULL;
// static TaskHandle_t CAN_Recieve_Task_Handle = NULL;

static void Creat_Task(void *arg);
static void Control_Task(void *arg);
static void UART_Recieve_Task(void *arg);
static void Test_Task(void * arg);
static void Sent_Val_Task(void * arg);
static void TCP_Server_Task(void * arg);
static void QR_Code_Info_Task(void * arg);
static void Parse_Send_Task(void * arg);
static void PC_Control_Task(void * arg);

static void TCP_Server_Init(void);
// static void CAN_Recieve_Task(void *arg);

int16_t speed1 = 0;
int16_t speed2 = 0;
int16_t speed3 = 0;
int16_t speed4 = 0;

char rx_buffer[128];
char addr_str[128];
int addr_family;
int ip_protocol;
int sock;
struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6

//CAN or UART 初始化引脚如果和摄像头引脚冲突，会使摄像头捕获图像失败，程序不停复位
//修改CAN UART引脚之后，摄像头可以捕获到图像，程序不会发生复位。但是在扫描图像时识别不出来二维码信息(Resize the QR-code recognizer err.)
//经试验：UART初始化会使摄像头扫描二维码失败(Resize the QR-code recognizer err.)不一定是IO引脚的问题，应该是时钟总线冲突的问题
//CAN初始化和摄像头不冲突
//So UART and camera cannot be used at the same time
//摄像头与CAN UART冲突已解决

//控制RGB目前会出现问题：程序复位，需重新扫描二维码   esp_wifi_connect 1134 wifi not start  原因：RGB引脚其中０.４与摄像头冲突
//目前只能单独控制GPIO2 LED，但是如果使用ＰＷＭ高速模式，摄像头扫描会报错　E (5737) camera: Timeout waiting for VSYNC　　　　E (5737) app-qrcode: Camera capture failed
//如果使用ＰＷＭ低速模式，E (4638) ledc: ledc_set_fade_with_time(678): target_duty argument is invalid　会导致程序复位

void app_main()
{
    wifi_config_t wifi_config;

    main_begin = xSemaphoreCreateBinary();
    // Initialize flash
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    app_gpio_init();

    app_initialise_wifi();

    skr_camera_init();

    ret = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    if (ret == ESP_OK && wifi_config.sta.ssid[0] != 0) {
        ESP_LOGI("main", "Connect to SSID:%s Password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        skr_start_wifi_connect(&wifi_config);
        wait_for_ip();
        xSemaphoreGive(main_begin);
    } else {
        ESP_LOGI("main", "Start QiFi netconfig");
        skr_start_app_qifi_task();
    }


    xSemaphoreTake(main_begin, portMAX_DELAY);
    ESP_LOGI("main", "the main begin %ds after", main_begin_wait_time);

    for(int i = main_begin_wait_time; i > 0; i--){
        printf("%d\r\n", i);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    vSemaphoreDelete(main_begin);

        
    TCP_Server_Init();

    user_Init();

    BaseType_t xReturn = pdPASS;
    xReturn = xTaskCreate((TaskFunction_t)Creat_Task, "Creat_Task", 4096, NULL, 10, &Creat_Task_Handle);
    if(xReturn == pdPASS){
        printf("Creat Task is success!\n\n\n"); 
    }
}

static void Creat_Task(void *arg)
{
    BaseType_t xReturn = pdPASS;

    xReturn = xTaskCreate((TaskFunction_t)TCP_Server_Task, "TCP_Server_Task", 1024*3, NULL, 5, &TCP_Server_Task_Handle);
    if(xReturn == pdPASS)
        printf("TCP_Server_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)UART_Recieve_Task, "UART_Recieve_Task", 1024*3, NULL, 5, &UART_Recieve_Task_Handle);
    if(xReturn == pdPASS)
        printf("UART_Recieve_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)Parse_Send_Task, "Parse_Send_Task", 1024*2, NULL, 5, &Parse_Send_Task_Handle);
    if(xReturn == pdPASS)
        printf("Parse_Send_Task Creat success!\n\n");
        
    // xReturn = xTaskCreate((TaskFunction_t)Control_Task, "Control_Task", 2048, NULL, 2, &Control_Task_Handle);
    // if(xReturn == pdPASS)
    //     printf("Control_Task Creat success!\n\n");

    // xReturn = xTaskCreate((TaskFunction_t)Sent_Val_Task, "Sent_Val_Task", 2048, NULL, 6, &Send_Val_Task_Handle);
    // if(xReturn == pdPASS)
    //     printf("Sent_Val_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)QR_Code_Info_Task, "QR_Code_Info_Task", 1024*2, NULL, 5, &QR_Code_Info_Task_Handle);
    if(xReturn == pdPASS)
        printf("QR_Code_Info_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)Test_Task, "Test_Task", 1024*3, NULL, 5, &Test_Task_Handle);
    if(xReturn == pdPASS)
        printf("Test_Task Creat success!\n\n");

    xReturn = xTaskCreate((TaskFunction_t)PC_Control_Task, "PC_Control_Task", 1024*3, NULL, 5, &PC_Control_Task_Handle);
    if(xReturn == pdPASS)
        printf("PC_Control_Task Creat success!\n\n");

    // xReturn = xTaskCreate((TaskFunction_t)CAN_Recieve_Task, "CAN_Recieve_Task", 2048, NULL, 6, &CAN_Recieve_Task_Handle);
    // if(xReturn == pdPASS)
    //     printf("CAN_Recieve_Task Creat success!\n\n");

    vTaskDelete(NULL);
}

static void Test_Task(void *arg)
{
    int cnt = 0;
    // camera_fb_t *fb = NULL;
    char Task_Buffer[800] = {0};
    while (1) {

        // Capture a frame
        // fb = esp_camera_fb_get();

        // if (!fb) {
        //     ESP_LOGE(TAG, "Camera capture failed");
        // }else{
        //     ESP_LOGE(TAG, "Camera capture succeed");
        // }

        vTaskList(Task_Buffer);
        printf("\n任务名      任务状态  优先级    剩余栈 任务序号\n");
        ESP_LOGI("***************Task_State_List***************", "\n%s", Task_Buffer);
        printf("%d", strlen(Task_Buffer));

        // ledc_set_fade_with_time(ledc_config.speed_mode,
        //     ledc_config.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
        // ledc_fade_start(ledc_config.speed_mode,
        //     ledc_config.channel, LEDC_FADE_NO_WAIT);
        // vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        // ledc_set_fade_with_time(ledc_config.speed_mode,
        //         ledc_config.channel, 0, LEDC_TEST_FADE_TIME);
        // ledc_fade_start(ledc_config.speed_mode,
        //         ledc_config.channel, LEDC_FADE_NO_WAIT);
        // vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        gpio_set_level(GPIO_NUM_2, cnt % 2);
        cnt++;
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

static void TCP_Server_Task(void * arg)
{
    char * instr;
    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        // Error occured during receiving
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        }
        // Connection closed
        else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed");
            break;
        }
        // Data received
        else {
            // Get the sender's ip address as string
            if (sourceAddr.sin6_family == PF_INET) {
                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
            } else if (sourceAddr.sin6_family == PF_INET6) {
                inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
            }

            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
            ESP_LOGI(TAG, "%s", rx_buffer);
/*****************************************************/
            instr = rx_buffer;
            int back = Rev_Parse(instr);

            if(back == 0){
                printf("err format,please input \n");
            }
/*****************************************************/
            int err = send(sock, rx_buffer, len, 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                break;
            }
        }
    }

    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket");
        shutdown(sock, 0);
        close(sock);
    }
    vTaskDelete(NULL);
}

static void Parse_Send_Task(void * arg)
{
    char QR_Info_buff[512];
    int len;
    char e[] = "East", s[] = "South", w[] = "West", n[] = "North";
    while (1) {
        if(QR_Code_Parse_OK_Flag == 1){
        
            len   = sprintf(QR_Info_buff,             "name:      %d\n", info->name);
            len += sprintf(QR_Info_buff + len, "self_x:    %d\n", info->self_x);
            len += sprintf(QR_Info_buff + len, "self_y:    %d\n", info->self_y);
            len += sprintf(QR_Info_buff + len, "self_z:    %d\n", info->self_z);
            len += sprintf(QR_Info_buff + len, "\n");

            if(info->target_t.E_target_name != 0){
                len += sprintf(QR_Info_buff + len, "E_direction:      %s\n", e);
                len += sprintf(QR_Info_buff + len, "E_target_name:    %d\n", info->target_t.E_target_name);
                len += sprintf(QR_Info_buff + len, "E_distance:       %d\n", info->target_t.E_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }
            if(info->target_t.S_target_name != 0){
                len += sprintf(QR_Info_buff + len, "S_direction:      %s\n", s);
                len += sprintf(QR_Info_buff + len, "S_target_name:    %d\n", info->target_t.S_target_name);
                len += sprintf(QR_Info_buff + len, "S_distance:       %d\n", info->target_t.S_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }
            if(info->target_t.W_target_name != 0){
                len += sprintf(QR_Info_buff + len, "W_direction:      %s\n", w);
                len += sprintf(QR_Info_buff + len, "W_target_name:    %d\n", info->target_t.W_target_name);
                len += sprintf(QR_Info_buff + len, "W_distance:       %d\n", info->target_t.W_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }
            if(info->target_t.N_target_name != 0){
                len += sprintf(QR_Info_buff + len, "N_direction:      %s\n", n);
                len += sprintf(QR_Info_buff + len, "N_target_name:    %d\n", info->target_t.N_target_name);
                len += sprintf(QR_Info_buff + len, "N_distance:       %d\n", info->target_t.N_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }

            len += sprintf(QR_Info_buff + len, "\n\n");
            printf("%d", len);
            if(len > 512){
                ESP_LOGE(TAG, "Error : ");
                break;
            }

            int err = send(sock, QR_Info_buff, len, 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                break;
            }

            QR_Code_Parse_OK_Flag = 0;
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

static void Sent_Val_Task(void *arg)
{
    while(1){
        if(ESCMode == ESCMode_Disable){
            C610_SendCurrentVal(0, 0, 0, 0);
            ESP_LOGI("ESC_Mode", "ESCMode: %d", ESCMode);
        }
        else{
            //  C610_VelocityControl(0, Underpan.Speed_Wheel_1);
            //  C610_VelocityControl(1, Underpan.Speed_Wheel_2);
            //  C610_VelocityControl(2, Underpan.Speed_Wheel_3);
            //  C610_VelocityControl(3, Underpan.Speed_Wheel_4);
            //  C610_SendCurrentVal(PID_ESC_Velocity[0].Output, PID_ESC_Velocity[1].Output,
            //                                                PID_ESC_Velocity[2].Output, PID_ESC_Velocity[3].Output);
            speed1 = (int16_t)(Underpan.Speed_Wheel_1 / 2);
            speed2 = (int16_t)(Underpan.Speed_Wheel_2 / 2);
            //电机电流限制
            if(abs(speed1) > 500){
                 C610_SendCurrentVal(0, 0, 0, 0);
            }else{
                 C610_SendCurrentVal(speed1, speed2, 0, 0);
            }

            ESP_LOGI("Test", "underpanspeed1: %d, underpanspeed2: %d", speed1, speed2);
            //  C610_SendCurrentVal(PID_ESC_Velocity[0].Output, 0, 0, 0);
            // ESP_LOGI("ESC_Mode", "ESCMode: %d", ESCMode);
            // ESP_LOGI("Velocity", "Velocity[1]: %f, Velocity[2]: %f, Velocity[3]: %f, Velocity[4]: %f", 
            //                         PID_ESC_Velocity[0].Output, 
            //                         PID_ESC_Velocity[1].Output, 
            //                         PID_ESC_Velocity[2].Output, 
            //                         PID_ESC_Velocity[3].Output);
            ESP_LOGI("Send", "Send is OK");
        }	
        vTaskDelay(20 / portTICK_RATE_MS);
    }
}

static void Control_Task(void *arg)
{
    //static int LY, RX, RY, T2, T1;
    while(1)
    {
        UnderpanControlTask();
        // LY = Remote_PulseToVal(Remote_GetLY());
        // RX = Remote_PulseToVal(Remote_GetRX());
        // RY = Remote_PulseToVal(Remote_GetRY());
        // T2 = Remote_PulseToSwitch(Remote_GetT2());
        // T1 = Remote_PulseToSwitch(Remote_GetT1());
        // ESP_LOGI("Remote", "LY: %d, RX: %d, RY: %d, T2: %d, T1: %d", LY, RX, RY, T2, T1);
        vTaskDelay(20 / portTICK_RATE_MS);
    }
}

static void UART_Recieve_Task(void *arg)
{
    int read_len;
    uart_event_t event;
    ESP_LOGI(TAG, "Hello_world");
    uint8_t* dtmp = (uint8_t*) malloc(1024);
    for(;;) {
        uint8_t flag = 1;
        //Waiting for UART event.
        xQueueReceive(uart1_queue, (void * )&event, (portTickType)portMAX_DELAY);
        bzero(dtmp, 1024);
        //ESP_LOGI("UART", "uart[%d] event:", UART_NUM_1);
        switch(event.type) {
            //Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                read_len = uart_read_bytes(UART_NUM_1, dtmp, event.size, portMAX_DELAY);
                //ESP_LOGI("UART", "Read %d bytes: %d, %d, %d, %d, %d", read_len, *dtmp, *(dtmp+1), *(dtmp+2), *(dtmp+3), *(dtmp+4));
                dtmp[read_len] = 0;
                const uint8_t *d = dtmp;
                static uint8_t RxState = 0, RxDataIndex = 0;
                while(flag){
                    switch(RxState){
                        case 0:				//还未检测到起始标志，开始匹配起始标志
                            if(*d == 0x0F)
                            {
                                RxState++;
                                SBUS_MsgPack[0] = *d;
                                // ESP_LOGI("UART", "SBUS_Msg[0]: %d",SBUS_MsgPack[0]);
                                RxDataIndex = 1;
                            }
                            else
                                RxState = 0;
                            break;
                        case 1:					//起始标志匹配，开始接收原始数据
                            SBUS_MsgPack[RxDataIndex] = *d;
                            // ESP_LOGI("UART", "SBUS_Msg[%d]: %d",RxDataIndex, SBUS_MsgPack[RxDataIndex]);
                            RxDataIndex++;
                            if(RxDataIndex >= 23)
                            {
                                RxDataIndex = 0;
                                RxState++;
                            }
                            break;
                        case 2:					//数据已经接收完毕，开始匹配结束标志第一字节
                                SBUS_MsgPack[23] = *d;
                                //ESP_LOGI("UART", "SBUS_Msg[23]: %d",SBUS_MsgPack[23]);
                                RxState++;
                            break;
                        case 3:					//结束标志第一字节已匹配，开始匹配结束标志第二字节
                            if(*d == 0x00)
                            {
                                UpdateRemoteInfo((void*)&SBUS_MsgPack[0]);
                                SBUS_MsgPack[24] = 0x00;
                                //ESP_LOGI("UART", "SBUS_Msg[24]: %d",SBUS_MsgPack[24]);
                                RemoteUpdated = 1;
                            }
                            RxState = 0;
                            flag = 0;
                            break;
                        default:
                            RxState = 0;
                    }
                    d++;
                }
                ESP_LOGI("UART", "Remote.Rx: %d, %d, %d, %d, %d, %d", SBUS_ChanelVal[0], SBUS_ChanelVal[1], SBUS_ChanelVal[3], SBUS_ChanelVal[4], SBUS_ChanelVal[5], SBUS_ChanelVal[6]);
                break;

            default:
                // ESP_LOGI("UART", "uart event type: %d", event.type);
                break;
            }
    }
    free(dtmp);
    dtmp = NULL;
}

static void TCP_Server_Init(void)
{
#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 destAddr;
    bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
    destAddr.sin6_family = AF_INET6;
    destAddr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket binded");

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occured during listen: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket listening");

    uint addrLen = sizeof(sourceAddr);
    sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket accepted");
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

// static void CAN_Recieve_Task(void *arg)
// {
//     while(1)
//     {   
//         can_message_t RxMsg;
//         can_receive(&RxMsg, portMAX_DELAY);
//         C610_GetMotorInfo(&RxMsg);
//         //vTaskDelay(50 / portTICK_RATE_MS);
//     }
// }

