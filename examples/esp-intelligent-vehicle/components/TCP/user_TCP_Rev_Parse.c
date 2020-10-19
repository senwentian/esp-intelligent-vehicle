#include "user_TCP_Rev_Parse.h"

static TaskHandle_t Remote_Control_Task_Handle = NULL;

static char *instr;
static char *str;
static int lenth;

int x, y;

Dst_address Dst_address_info;
Dst_address *Dst_info;
wayfind_t wayfinding;

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
        // printf("%d\r\n", Dst_info->Dst_x);
        instr = instr + lenth + strlen(".");
        Dst_info->Dst_y = atol(instr);
        // printf("%d\r\n", Dst_info->Dst_y);
        x = Dst_info->Dst_x;
        y = Dst_info->Dst_y;
#if CONFIG_PC_CONTROL_OPERATION_MODE
        xTaskNotify(Remote_Control_Task_Handle, BIT_Dst, eSetBits);
#endif
        return 1;
    }
    else if (!strcmp(str, "right"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->E_Distance = atol(instr);
        // printf("%d\r\n", Dst_info->E_Distance);
#if CONFIG_PC_CONTROL_OPERATION_MODE
        xTaskNotify(Remote_Control_Task_Handle, BIT_right, eSetBits);
#endif
        return 1;
    }
    else if (!strcmp(str, "behind"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->S_Distance = atol(instr);
        // printf("%d\r\n", Dst_info->S_Distance);
#if CONFIG_PC_CONTROL_OPERATION_MODE
        xTaskNotify(Remote_Control_Task_Handle, BIT_behind, eSetBits);
#endif     
        return 1;
    }
    else if (!strcmp(str, "left"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->W_Distance = atol(instr);
        // printf("%d\r\n", Dst_info->W_Distance);
#if CONFIG_PC_CONTROL_OPERATION_MODE
        xTaskNotify(Remote_Control_Task_Handle, BIT_left, eSetBits);
#endif
        return 1;
    }
    else if (!strcmp(str, "front"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->N_Distance = atol(instr);
        // printf("%d\r\n", Dst_info->N_Distance);
#if CONFIG_PC_CONTROL_OPERATION_MODE
        xTaskNotify(Remote_Control_Task_Handle, BIT_front, eSetBits);
#endif
        return 1;
    }
    else if (!strcmp(str, "pickup"))
    {
#if CONFIG_PC_CONTROL_OPERATION_MODE
        xTaskNotify(Remote_Control_Task_Handle, BIT_pickup, eSetBits);
#else
        Pick_up_Action();
#endif
        return 1;
    }
    else if (!strcmp(str, "putdown"))
    {
#if CONFIG_PC_CONTROL_OPERATION_MODE
        xTaskNotify(Remote_Control_Task_Handle, BIT_putdown, eSetBits);
#else
        Put_down_Action();
#endif
        return 1;
    }
    else if (!strcmp(str, "help"))
    {
        int back = help_menu();
        if (back > 0) {
            return 1;
        }
        return 3;
    }
    return 2;
}

/*
               N
        4     1      7
                |
  W  3----------6  E
                |
        5     2       8
               S
*/
void Automatic_wayfind_easy(void)
{
    if(wayfinding.finish_flag != -1)
    {
        int Dir = direction_num;
        switch (Dir)
        {
        case front_num:
            QR_Current_Action(info->target_t.N_distance, 1, 1);
            break;
        
        case behind_num:
            QR_Current_Action(info->target_t.S_distance, -1, 1);
            break;

        case left_num:
            QR_Current_Action(info->target_t.W_distance, -1, -1);
            break;

        case right_num:
            QR_Current_Action(info->target_t.E_distance, 1, -1);
            break;

        case left_front_Num:
            Oblique_Action(info->target_t.W_distance, info->target_t.N_distance, -1, -1, 1, 1);
            break;

        case left_behind_Num:
            Oblique_Action(info->target_t.W_distance, info->target_t.S_distance, -1, -1, -1, 1);
            break;

        case right_front_Num:
            Oblique_Action(info->target_t.E_distance, info->target_t.N_distance, 1, -1, 1, 1);
            break;

        case right_behind_Num:
            Oblique_Action(info->target_t.E_distance, info->target_t.S_distance, 1, -1, -1, 1);
            break;

        default:
            wayfinding.finish_flag = -1;
            break;
        }
        wayfinding.num++;
    }
}

void Automatic_wayfind(void)
{
    if(wayfinding.finish_flag != -1)
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
            wayfinding.finish_flag = -1;
        }
        wayfinding.num++;
    }
}

void Remote_Control_Task(void *arg)
{
    BaseType_t xResult;
    uint32_t ulValue;
    wayfinding.num = 0;
    wayfinding.finish_flag = 0;
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
                // if (x > 0)
                // {
                //     Current_Action(x, 1, -1);
                // }
                // else
                // {
                //     Current_Action(-x, -1, -1);
                // }
                // if (y > 0)
                // {
                //     Current_Action(y, 1, 1);
                // }
                // else
                // {
                //     Current_Action(-y, -1, 1);
                // }
                    if(x > 0 && y > 0)
                        Oblique_Action(x, y, 1, -1, 1, 1);
                    else if(x > 0 && y < 0)
                        Oblique_Action(x, -y, 1, -1, -1, 1);
                    else if(x < 0 && y > 0)
                        Oblique_Action(-x, y, -1, -1, 1, 1);
                    else if(x < 0 && y < 0)
                        Oblique_Action(-x, -y, -1, -1, -1, 1);
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
}


