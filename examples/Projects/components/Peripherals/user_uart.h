#ifndef  _USER_UART_H
#define _USER_UART_H

#include "string.h"
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


extern QueueHandle_t uart1_queue;
static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (12)
#define RXD_PIN (13)

#define TAG "Project"

void uart_Init(void);




#endif
