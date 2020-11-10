#ifndef _SEND_CURRENT_H
#define _SEND_CURRENT_H

#include <stdio.h>
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"

#include "esp_err.h"
#include "esp_log.h"

#include "user_motor.h"
#include "user_control.h"
#include "config.h"

void Send_Current_Create(void);

void PC_Send_Current(void);

void Current_Action(uint32_t time_num, int sym1, int sym2);

void Spin_Current_Action(uint32_t Times, int sym);

void QR_Current_Action(uint32_t times, int sym1, int sym2);

void Oblique_Action(uint32_t x, uint32_t y, int sym1_x, int sym2_x, int sym1_y, int sym2_y);

#endif
