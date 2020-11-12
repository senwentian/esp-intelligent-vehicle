#ifndef  _SEND_CURRENT_H
#define _SEND_CURRENT_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"

#include "esp_err.h"
#include "esp_log.h"

#include "user_motor.h"
#include "user_control.h"

static TaskHandle_t Send_Current_Task_Handle = NULL;
static TaskHandle_t PC_Send_Current_Task_Handle = NULL;

void Send_Current(void);

void PC_Send_Current(void);






#endif
