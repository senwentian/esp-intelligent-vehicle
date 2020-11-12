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

void TCP_Rev_parser_init(Dst_address *parser)
{
    if (parser)
    {
        memset(parser, 0x0, sizeof(Dst_address));
    }
    return;
}

static char *parse_elem(char *instr, char *search_str, int *buf_size)
{
    char *str = NULL;
    int len = 0;
    static char buf[256] = {0};

    str = strstr(instr, search_str);
    if (!str)
    {
        len = 0;
        return NULL;
    }
    len = str - instr;
    memset(buf, 0, sizeof(buf));
    memcpy(buf, instr, len);
    *buf_size = strlen(buf);
    //printf("buf:%s\n",buf);
    return (char *)buf;
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
        xTaskNotify(Remote_Control_Task_Handle, BIT_4, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "right"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->E_Distance = atol(instr);
        printf("%d\r\n", Dst_info->E_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_0, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "behind"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->S_Distance = atol(instr);
        printf("%d\r\n", Dst_info->S_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_1, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "left"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->W_Distance = atol(instr);
        printf("%d\r\n", Dst_info->W_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_2, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "front"))
    {
        instr = instr + lenth + strlen(":");
        Dst_info->N_Distance = atol(instr);
        printf("%d\r\n", Dst_info->N_Distance);
        xTaskNotify(Remote_Control_Task_Handle, BIT_3, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "pickup"))
    {
        xTaskNotify(Remote_Control_Task_Handle, BIT_5, eSetBits);
        return 1;
    }
    else if (!strcmp(str, "putdown"))
    {
        xTaskNotify(Remote_Control_Task_Handle, BIT_6, eSetBits);
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
  * front     |      1      |      1       |
  * behind |     -1     |      1       |
  * left        |     -1      |      -1     |
  * right     |      1      |      -1     |
  */
void Current_Action(uint32_t time_num, int sym1, int sym2)
{
    printf("see it time_num:%d", time_num);
    for(int i = 0; i < time_num; i++)
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

void Remote_Control_Task(void *arg)
{
    BaseType_t xResult;
    uint32_t ulValue;
    while (1)
    {
        xResult = xTaskNotifyWait(0x00000000, 0xFFFFFFFF, &ulValue, portMAX_DELAY);
        if (xResult == pdPASS)
        {
            switch (ulValue)
            {
            case BIT_0:
                Current_Action(Dst_info->E_Distance, 1, -1);
                break;

            case BIT_1:
                Current_Action(Dst_info->S_Distance, -1, 1);
                break;

            case BIT_2:
                Current_Action(Dst_info->W_Distance, -1, -1);
                break;

            case BIT_3:
                Current_Action(Dst_info->N_Distance, 1, 1);
                break;

            case BIT_4:
                if(x > 0){
                    Current_Action(x, 1, -1);
                }else{
                    Current_Action(-x, -1, -1);
                }
                if(y > 0){
                    Current_Action(y, 1, 1);
                }else{
                    Current_Action(-y, -1, 1);
                }
                printf("Dstaddress hello world\n");
                break;

            case BIT_5:
                Pick_up_Action();
                break;

            case BIT_6:
                Put_down_Action();
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
