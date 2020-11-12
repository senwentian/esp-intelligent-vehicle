#ifndef  _USER_TCP_REV_PARSE_H
#define _USER_TCP_REV_PARSE_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_types.h"
#include "esp_log.h"
#include "esp_system.h"

#include "user_motor.h"

#define BIT_0    (1ULL)
#define BIT_1    (1ULL << 1)
#define BIT_2    (1ULL << 2)
#define BIT_3    (1ULL << 3)
#define BIT_4    (1ULL << 4)
#define BIT_5    (1ULL << 5)
#define BIT_6    (1ULL << 6)
#define BIT_7    (1ULL << 7)

typedef enum{
    positive = 1,
    negative = -1
}symbol_t;

typedef struct{
    uint32_t Dst_x;
    uint32_t Dst_y;
    uint32_t Dst_z;

    uint32_t E_Distance;
    uint32_t S_Distance;
    uint32_t W_Distance;
    uint32_t N_Distance;

}Dst_address;

extern Dst_address * Dst_info;



int Rev_Parse(char * instr);

void PC_Remote_Control_Task(void);





#endif
