#include "user_can.h"


void can_Init(void)
{
     //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_14, GPIO_NUM_15,CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_1MBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    //Install can driver
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("CAN Driver installed\n");
    } else {
        printf("Failed to install the CAN driver\n");
        return;
    }

    //Start can driver
    if (can_start() == ESP_OK) {
        printf("CAN Driver started\n"); 
    } else {
        printf("Failed to start the CAN driver\n");
        return;
    }
}

