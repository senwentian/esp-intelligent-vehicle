#ifndef _USER_PWM_H
#define _USER_PWM_H

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include "user_remote.h"
#include "config.h"

void pwm_Init(void);

void mcpwm_control(void);

void Put_down_Action(void);

void Pick_up_Action(void);

#endif
