#include "send_current.h"

int16_t speed1 = 0;
int16_t speed2 = 0;
int16_t speed3 = 0;
int16_t speed4 = 0;

void Send_Current_Task(void *arg)
{
    while (1)
    {
        if (ESCMode == ESCMode_Disable)
        {
            C610_SendCurrentVal(0, 0, 0, 0);
            ESP_LOGI("ESC_Mode", "ESCMode: %d", ESCMode);
        }
        else
        {
            //  C610_VelocityControl(0, Underpan.Speed_Wheel_1);
            //  C610_VelocityControl(1, Underpan.Speed_Wheel_2);
            //  C610_VelocityControl(2, Underpan.Speed_Wheel_3);
            //  C610_VelocityControl(3, Underpan.Speed_Wheel_4);
            //  C610_SendCurrentVal(PID_ESC_Velocity[0].Output, PID_ESC_Velocity[1].Output,
            //                                                PID_ESC_Velocity[2].Output, PID_ESC_Velocity[3].Output);
            speed1 = (int16_t)(Underpan.Speed_Wheel_1 / 2);
            speed2 = (int16_t)(Underpan.Speed_Wheel_2 / 2);
            speed3 = (int16_t)(Underpan.Speed_Wheel_3 / 2);
            speed4 = (int16_t)(Underpan.Speed_Wheel_4 / 2);
            //电机电流限制
            if (abs(speed1) > 700)
            {
                ESP_LOGI("Test0", "underpanspeed1: 0, underpanspeed2: 0, underpanspeed3: 0, underpanspeed4: 0");
                C610_SendCurrentVal(0, 0, 0, 0);
            }
            else
            {
                ESP_LOGI("Test1", "underpanspeed1: %d, underpanspeed2: %d, underpanspeed3: %d, underpanspeed4: %d", speed1, speed2, speed3, speed4);
                C610_SendCurrentVal(speed1, speed2, speed3, speed4);
            }
            //  C610_SendCurrentVal(PID_ESC_Velocity[0].Output, 0, 0, 0);
            // ESP_LOGI("ESC_Mode", "ESCMode: %d", ESCMode);
            // ESP_LOGI("Velocity", "Velocity[1]: %f, Velocity[2]: %f, Velocity[3]: %f, Velocity[4]: %f",
            //                         PID_ESC_Velocity[0].Output,
            //                         PID_ESC_Velocity[1].Output,
            //                         PID_ESC_Velocity[2].Output,
            //                         PID_ESC_Velocity[3].Output);
            // ESP_LOGI("Send", "Send is OK");
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void PC_Send_Current_Task(void)
{
    // int flag  = eTaskGetState();
    // vTaskSuspend(Send_Current_Task_Handle);
    // vTaskResume(PC_Send_Current_Task_Handle);

}

void Send_Current(void)
{
    BaseType_t xReturn = pdPASS;

    xReturn = xTaskCreate((TaskFunction_t)Send_Current_Task, "Send_Current_Task", 1024 * 3, NULL, 6, &Send_Current_Task_Handle);
    if (xReturn == pdPASS)
        printf("Send_Current_Task Creat Success!\n");
}

void PC_Send_Current(void)
{
    BaseType_t xReturn = pdPASS;

    xReturn = xTaskCreate((TaskFunction_t)PC_Send_Current_Task, "PC_Send_Current_Task", 1024 * 3, NULL, 6, &PC_Send_Current_Task_Handle);
    if (xReturn == pdPASS)
        printf("PC_Send_Current_Task Creat Success!\n");
}

