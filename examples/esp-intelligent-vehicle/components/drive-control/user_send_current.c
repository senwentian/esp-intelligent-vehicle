#include "user_send_current.h"

static TaskHandle_t Send_Current_Task_Handle = NULL;
static TaskHandle_t PC_Send_Current_Task_Handle = NULL;

#define Val (480)
#define Half_Val (Val / 2)

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
            //Current protection
            if (abs(speed1) > 700)
            {
                ESP_LOGI("Test0", "underpanspeed1: 0, underpanspeed2: 0, underpanspeed3: 0, underpanspeed4: 0");
                C610_SendCurrentVal(300, 300, 300, 300);
            }
            else
            {
                ESP_LOGI("Test1", "underpanspeed1: %d, underpanspeed2: %d, underpanspeed3: %d, underpanspeed4: %d", speed1, speed2, speed3, speed4);
                C610_SendCurrentVal(speed1, speed2, speed3, speed4);
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void Sent_DstVal(int start, int DstVal, int step, int one, int two, int three, int four)
{
    int val = start;
    while (1)
    {
        if ((val + step) < DstVal)
        {
            val += step;
            C610_SendCurrentVal(one * val, two * val, three * val, four * val);
        }
        else
        {
            val = DstVal;
            C610_SendCurrentVal(one * val, two * val, three * val, four * val);
            break;
        }
    }
}

void Oblique_Action(uint32_t x, uint32_t y, int sym1_x, int sym2_x, int sym1_y, int sym2_y)
{
    int x_Speed = x > y ? Val : Val * x / y;
    int y_Speed = x > y ? Val * y / x : Val;
    int time = x > y ? x : y;
    int16_t Auto_Wheel1 = sym1_x * x_Speed + sym1_y * y_Speed;
    int16_t Auto_Wheel2 = sym1_x * sym2_x * x_Speed + sym1_y * sym2_y * y_Speed;
    int16_t Auto_Wheel3 = sym1_x * -x_Speed + sym1_y * -y_Speed;
    int16_t Auto_Wheel4 = sym1_x * sym2_x * -x_Speed + sym1_y * sym2_y * -y_Speed;
    printf("Auto_Wheel1:%d; Auto_Wheel2:%d; Auto_Wheel3:%d; Auto_Wheel4:%d", Auto_Wheel1, Auto_Wheel2, Auto_Wheel3, Auto_Wheel4);
    if(abs(Auto_Wheel1) < 1000 && abs(Auto_Wheel2) < 1000 && abs(Auto_Wheel3) < 1000 && abs(Auto_Wheel4) < 1000)
    {
        for(int i = 0; i < time; i++)
        {
            C610_SendCurrentVal(Auto_Wheel1/2, Auto_Wheel2/2, Auto_Wheel3/2, Auto_Wheel4/2);
            vTaskDelay(15 / portTICK_RATE_MS);
            printf("Send is OK : time : %d\n", i);
        }
        C610_SendCurrentVal(Auto_Wheel1/4, Auto_Wheel2/4, Auto_Wheel3/4, Auto_Wheel4/4);
        vTaskDelay(100 / portTICK_RATE_MS);
        C610_SendCurrentVal(0, 0, 0, 0);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

/*
  *                |  sym1  |  sym2  |
  * front     |      1      |      1       | North
  * behind |     -1     |      1       | Sorth
  * left        |     -1      |      -1     | West
  * right     |      1      |      -1      | East
  */
void QR_Current_Action(uint32_t times, int sym1, int sym2)
{
    printf("\r\nsee it times:%d\r\n", times);
    for (int i = 0; i < times; i++)
    {
        C610_SendCurrentVal(sym1 * Val,
                            sym1 * sym2 * Val,
                            sym1 * -Val,
                            sym1 * sym2 * -Val);
        vTaskDelay(15 / portTICK_RATE_MS);
        printf("Send is OK:times:%d\n", i);
    }
    C610_SendCurrentVal(sym1 * Half_Val,
                        sym1 * sym2 * Half_Val,
                        sym1 * -Half_Val,
                        sym1 * sym2 * -Half_Val);
    vTaskDelay(100 / portTICK_RATE_MS);
    C610_SendCurrentVal(0, 0, 0, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
}

/*
  *                |  sym1  |  sym2  |
  * front     |      1      |      1       | North
  * behind |     -1     |      1       | Sorth
  * left        |     -1      |      -1     | West
  * right     |      1      |      -1      | East
  */
void Current_Action(uint32_t Times, int sym1, int sym2)
{
    uint32_t time_num = Times * 10;
    printf("\r\nsee it time_num:%d\r\n", time_num);
    for (int i = 0; i < time_num; i++)
    {
        C610_SendCurrentVal(sym1 * Val,
                            sym1 * sym2 * Val,
                            sym1 * -Val,
                            sym1 * sym2 * -Val);
        vTaskDelay(100 / portTICK_RATE_MS);
        printf("Send is OK:time_num:%d\n", i);
    }
    C610_SendCurrentVal(sym1 * Half_Val,
                        sym1 * sym2 * Half_Val,
                        sym1 * -Half_Val,
                        sym1 * sym2 * -Half_Val);
    vTaskDelay(100 / portTICK_RATE_MS);
    C610_SendCurrentVal(0, 0, 0, 0);
    vTaskDelay(1000 / portTICK_RATE_MS);
}

void Spin_Current_Action(uint32_t Times, int sym)
{
    uint32_t time_num = Times * 10;
    for (int i = 0; i < time_num; i++)
    {
        C610_SendCurrentVal(sym * Val,
                            sym * Val,
                            sym * Val,
                            sym * Val);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    C610_SendCurrentVal(sym * Half_Val,
                        sym * Half_Val,
                        sym * Half_Val,
                        sym * Half_Val);
    vTaskDelay(100 / portTICK_RATE_MS);
    C610_SendCurrentVal(0, 0, 0, 0);
}

void Send_Current_Create(void)
{
    BaseType_t xReturn = pdPASS;

    xReturn = xTaskCreate((TaskFunction_t)Send_Current_Task, "Send_Current_Task", 1024 * 2, NULL, 6, &Send_Current_Task_Handle);
    if (xReturn == pdPASS)
        printf("Send_Current_Task Creat Success!\n");
}
