#include "user_TCP_Rev_Parse.h"

#define Val (400)
#define Half_Val (Val / 2)

static TaskHandle_t Remote_Control_Task_Handle = NULL;

static char *instr;
static char *str;
static int lenth;

int x, y;

Dst_address Dst_address_info;
Dst_address *Dst_info;
wayfind_t wayfinding;

static void Sent_DstVal(int start, int DstVal, int step, int one, int two, int three, int four)
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

static void TCP_Rev_parser_init(Dst_address *parser)
{
    if (parser)
    {
        memset(parser, 0x0, sizeof(Dst_address));
    }
    return;
}

void QR_Control(void)
{
    xTaskNotify(Remote_Control_Task_Handle, BIT_QR, eSetBits);
}

int Rev_Parse(char *instring)
{
    TCP_Rev_parser_init(&Dst_address_info);

    Dst_info = &Dst_address_info;

    instr = instring;
    str = NULL;
    lenth = 0;

    if ((!instr) || (!Dst_info))
    {
        return 3;
    }

    str = parse_elem(instr, ":", &lenth);
    if (str == NULL)
    {
        return 0;
    }

    if (!strcmp(str, "Dstaddress"))
    {
        instr = instr + lenth + strlen(":");
        str = parse_elem(instr, ".", &lenth);
        Dst_info->Dst_x = atol(str);
        printf("%d\r\n", Dst_info->Dst_x);
        instr = instr + lenth + strlen(".");
        Dst_info->Dst_y = atol(instr);
        printf("%d\r\n", Dst_info->Dst_y);
        x = Dst_info->Dst_x;
        y = Dst_info->Dst_y;
        xTaskNotify(Remote_Control_Task_Handle, BIT_Dst, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "right"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->E_Distance = atol(instr);
        printf("%d\r\n", Dst_info->E_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_right, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "behind"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->S_Distance = atol(instr);
        printf("%d\r\n", Dst_info->S_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_behind, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "left"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->W_Distance = atol(instr);
        printf("%d\r\n", Dst_info->W_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_left, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "front"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->N_Distance = atol(instr);
        printf("%d\r\n", Dst_info->N_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_front, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "pickup"))
    {
        xTaskNotify(Remote_Control_Task_Handle, BIT_pickup, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "putdown"))
    {
        xTaskNotify(Remote_Control_Task_Handle, BIT_putdown, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "telecontrol"))
    {
        // vTaskSuspend(Remote_Control_Task_Handle);
        // vTaskResume(Send_Val_Task_Handle);
        return 1;
    }
    else if (!strcmp(str, "PCcontrol"))
    {
        // vTaskSuspend(Send_Val_Task_Handle);
        // vTaskResume(Remote_Control_Task_Handle);
        return 1;
    }

    return 2;
}

/*
  *                |  sym1  |  sym2  |
  * front     |      1      |      1       | North
  * behind |     -1     |      1       | Sorth
  * left        |     -1      |      -1     | West
  * right     |      1      |      -1      | East
  */
void Current_Action(uint32_t time_num, int sym1, int sym2)
{
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

void Automatic_wayfind_easy(void)
{
    // printf("\r\ninfo->name: %d\r\n", info->name);
    // printf("info->self_x: %d; info->self_y: %d; info->self_z: %d\n", info->self_x, info->self_y, info->self_z);
    // printf("E_direction: %c\nE_target_name: %d\nE_distance: %d\n", info->target_t.E_direction, info->target_t.E_target_name, info->target_t.E_distance);
    // printf("W_direction: %c\nW_target_name: %d\nW_distance: %d\n", info->target_t.W_direction, info->target_t.W_target_name, info->target_t.W_distance);
    // printf("N_direction: %c\nN_target_name: %d\nN_distance: %d\n", info->target_t.N_direction, info->target_t.N_target_name, info->target_t.N_distance);
    // printf("S_direction: %c\nS_target_name: %d\nS_distance: %d\n", info->target_t.S_direction, info->target_t.S_target_name, info->target_t.S_distance);
    if(wayfinding.next_target_name != -1)
    {
        if (info->target_t.E_target_name > 0)
        {
            Current_Action(info->target_t.E_distance, 1, -1);
        }
        else if (info->target_t.N_target_name > 0)
        {
            Current_Action(info->target_t.N_distance, 1, 1);
        }
        else if(info->target_t.S_target_name > 0)
        {
            Current_Action(info->target_t.S_distance, -1, 1);
        }
        else if(info->target_t.W_target_name > 0)
        {
            Current_Action(info->target_t.W_distance, -1, -1);
        }else
        {
            wayfinding.next_target_name = -1;
        }
        wayfinding.num++;
    }
}

void Automatic_wayfind(void)
{
    if(wayfinding.next_target_name != -1)
    {
        if (info->target_t.E_target_name > 0)
        {
            Current_Action(info->target_t.E_distance, 1, -1);
        }
        else if (info->target_t.N_target_name > 0)
        {
            Current_Action(info->target_t.N_distance, 1, 1);
        }
        else if(info->target_t.S_target_name > 0)
        {
            Current_Action(info->target_t.S_distance, -1, 1);
        }
        else if(info->target_t.W_target_name > 0)
        {
            Current_Action(info->target_t.W_distance, -1, -1);
        }
        else
        {
            wayfinding.next_target_name = -1;
        }
        wayfinding.num++;
    }
}

void Remote_Control_Task(void *arg)
{
    BaseType_t xResult;
    uint32_t ulValue;
    wayfinding.num = 0;
    wayfinding.next_target_name = 0;
    while (1)
    {
        xResult = xTaskNotifyWait(0x00000000, 0xFFFFFFFF, &ulValue, portMAX_DELAY);
        if (xResult == pdPASS)
        {
            switch (ulValue)
            {
            case BIT_right:
                Current_Action(Dst_info->E_Distance, 1, -1);
                break;

            case BIT_behind:
                Current_Action(Dst_info->S_Distance, -1, 1);
                break;

            case BIT_left:
                Current_Action(Dst_info->W_Distance, -1, -1);
                break;

            case BIT_front:
                Current_Action(Dst_info->N_Distance, 1, 1);
                break;

            case BIT_Dst:
                if (x > 0)
                {
                    Current_Action(x, 1, -1);
                }
                else
                {
                    Current_Action(-x, -1, -1);
                }
                if (y > 0)
                {
                    Current_Action(y, 1, 1);
                }
                else
                {
                    Current_Action(-y, -1, 1);
                }
                break;

            case BIT_pickup:
                Pick_up_Action();
                break;

            case BIT_putdown:
                Put_down_Action();
                break;

            case BIT_QR:
                Automatic_wayfind_easy();
                break;

            default:
                break;
            }
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void PC_Remote_Control_Task(void)
{
    BaseType_t xReturn = pdPASS;
    xReturn = xTaskCreate(Remote_Control_Task, "Remote_Control_Task", 1024 * 2, NULL, 5, &Remote_Control_Task_Handle);
    if (xReturn == pdPASS)
        printf("Remote_Control_Task create success!\r\n");

    // vTaskSuspend(Remote_Control_Task_Handle);
}
