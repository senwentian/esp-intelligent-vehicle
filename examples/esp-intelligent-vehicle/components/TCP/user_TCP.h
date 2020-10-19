#ifndef _USER_TCP_INIT_H
#define _USER_TCP_INIT_H

#include "stdio.h"
#include "strings.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "user_qrcode_task.h"
#include "user_qrcode_info.h"

#include "user_TCP_Rev_Parse.h"
#include "config.h"

#define PORT CONFIG_PORT

void TCP_Server_Init(void);

void Parse_Send_Task_Create(void);

void TCP_Server_Task_Create(void);

int help_menu(void);

void QR_code_invaild_report(void);

#endif
