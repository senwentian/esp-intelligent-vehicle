/**
 *
 * ESP-Intelligent-Vehicle Firmware
 * 
 * Copyright 2019-2020  Espressif Systems (Shanghai) 
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * config.h - Main configuration file
 *
 * This file define the default configuration of the copter
 * It contains two types of parameters:
 * - The global parameters are globally defined and independent of any
 *   compilation profile. An example of such define could be some pinout.
 * - The profiled defines, they are parameter that can be specific to each
 *   dev build. The vanilla build is intended to be a "customer" build without
 *   fancy spinning debugging stuff. The developers build are anything the
 *   developer could need to debug and run his code/crazy stuff.
 *
 * The golden rule for the profile is NEVER BREAK ANOTHER PROFILE. When adding a
 * new parameter, one shall take care to modified everything necessary to
 * preserve the behavior of the other profiles.
 *
 * For the flag. T_ means task. H_ means HAL module. U_ would means utils.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// Task priorities. Higher number higher priority
#define SLOW_FREQUENCY_TASK_PRI     6
#define FAST_FREQUENCY_TASK_PRI        6
#define OTA_TASK_PRI            6
#define CREAT_TASK_PRI           5
#define TEST_TASK_PRI    5
#define CONTROL_TASK_PRI        5
#define CAN_RECIEVE_TASK_PRI        6
#define TCP_RECIEVE_TASK_PRI          5
#define QRCODE_PARSE_SEND_TASK_PRI        5
#define SEND_CURRENT_TASK_PRI       6
#define MCPWM_SERVO_CONTROL_TASK_PRI      5
#define UART_RECIEVE_TASK_PRI             5
#define QR_CODE_INFO_TASK_PRI         5
#define REMOTE_CONTROL_TASK_PRI       5


#define configBASE_STACK_SIZE CONFIG_BASE_STACK_SIZE

//Task stack sizes
#define SLOW_FREQUENCY_TASK_STACKSIZE         (500)
#define FAST_FREQUENCY_TASK_STACKSIZE            (500)
#define OTA_TASK_STACKSIZE             (8 * configBASE_STACK_SIZE)
#define CREAT_TASK_STACKSIZE        (4 * configBASE_STACK_SIZE)
#define TEST_TASK_STACKSIZE        (2* configBASE_STACK_SIZE)
#define CONTROL_TASK_STACKSIZE     (3 * configBASE_STACK_SIZE)
#define CAN_RECIEVE_TASK_STACKSIZE            (2*configBASE_STACK_SIZE)
#define TCP_RECIEVE_TASK_STACKSIZE            (2 * configBASE_STACK_SIZE)
#define QRCODE_PARSE_SEND_TASK_STACKSIZE          (2 * configBASE_STACK_SIZE)
#define SEND_CURRENT_TASK_STACKSIZE        (3 * configBASE_STACK_SIZE)
#define MCPWM_SERVO_CONTROL_TASK_STACKSIZE     (2 * configBASE_STACK_SIZE)
#define UART_RECIEVE_TASK_STACKSIZE     (3 * configBASE_STACK_SIZE)
#define QR_CODE_INFO_TASK_STACKSIZE       (64 * configBASE_STACK_SIZE)
#define REMOTE_CONTROL_TASK_STACKSIZE        (2 * configBASE_STACK_SIZE)


// Task names
#define SYSTEM_TASK_NAME        "SYSTEM"
#define ADC_TASK_NAME           "ADC"
#define PM_TASK_NAME            "PWRMGNT"
#define CRTP_TX_TASK_NAME       "CRTP-TX"
#define CRTP_RX_TASK_NAME       "CRTP-RX"
#define CRTP_RXTX_TASK_NAME     "CRTP-RXTX"
#define LOG_TASK_NAME           "LOG"
#define MEM_TASK_NAME           "MEM"
#define PARAM_TASK_NAME         "PARAM"
#define SENSORS_TASK_NAME       "SENSORS"
#define STABILIZER_TASK_NAME    "STABILIZER"
#define NRF24LINK_TASK_NAME     "NRF24LINK"
#define ESKYLINK_TASK_NAME      "ESKYLINK"
#define SYSLINK_TASK_NAME       "SYSLINK"
#define USBLINK_TASK_NAME       "USBLINK"
#define WIFILINK_TASK_NAME       "WIFILINK"
#define UDP_TX_TASK_NAME "UDP_TX"
#define UDP_RX_TASK_NAME  "UDP_RX"
#define UDP_RX2_TASK_NAME  "UDP_RX2"
#define PROXIMITY_TASK_NAME     "PROXIMITY"
#define EXTRX_TASK_NAME         "EXTRX"
#define UART_RX_TASK_NAME       "UART"
#define ZRANGER_TASK_NAME       "ZRANGER"
#define ZRANGER2_TASK_NAME      "ZRANGER2"
#define FLOW_TASK_NAME          "FLOW"
#define USDLOG_TASK_NAME        "USDLOG"
#define USDWRITE_TASK_NAME      "USDWRITE"
#define PCA9685_TASK_NAME       "PCA9685"
#define CMD_HIGH_LEVEL_TASK_NAME "CMDHL"
#define MULTIRANGER_TASK_NAME   "MR"
#define BQ_OSD_TASK_NAME        "BQ_OSDTASK"
#define GTGPS_DECK_TASK_NAME    "GTGPS"
#define LIGHTHOUSE_TASK_NAME    "LH"
#define LPS_DECK_TASK_NAME      "LPS"
#define OA_DECK_TASK_NAME       "OA"
#define UART1_TEST_TASK_NAME    "UART1TEST"
#define UART2_TEST_TASK_NAME    "UART2TEST"
#define KALMAN_TASK_NAME        "KALMAN"
#define ACTIVE_MARKER_TASK_NAME "ACTIVEMARKER-DECK"
#define AI_DECK_GAP_TASK_NAME   "AI-DECK-GAP"
#define AI_DECK_NINA_TASK_NAME  "AI-DECK-NINA"
#define UART2_TASK_NAME         "UART2"


#endif /* CONFIG_H_ */
