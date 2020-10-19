#ifndef  _USER_OTA_H
#define _USER_OTA_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "user_init.h"
#include "user_qrcode_task.h"
#include "config.h"


void user_ota_task(void);

#endif