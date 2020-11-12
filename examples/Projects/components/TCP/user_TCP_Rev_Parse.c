#include "user_TCP_Rev_Parse.h"

#define Val 300

static TaskHandle_t Remote_Control_Task_Handle = NULL;

char Dst[] = "Dstaddress";
char E[] = "right";
char S[] = "behind";
char W[] = "left";
char N[] = "front";

static char * instr;
static char * str;
static int lenth;

Dst_address Dst_address_info;
Dst_address * Dst_info;

static void Sent_DstVal(int start, int DstVal, int step, symbol_t one, symbol_t two, symbol_t three, symbol_t four)
{
    int val = start;
    while(1){
        if((val + step) < DstVal){
            val += step;
            C610_SendCurrentVal(one * val, two * val, three * val, four * val);
        }else{
            val = DstVal;
            C610_SendCurrentVal(one * val, two * val, three * val, four * val);
            break;
        }

    }
}

void TCP_Rev_parser_init(Dst_address *parser)
{
    if (parser) {
        memset(parser, 0x0, sizeof(Dst_address));
    }
    return;
}

static char *parse_elem(char * instr,char *search_str,int *buf_size)
{
    char *str = NULL;
    int len = 0;
    static char buf[256] = {0};

    str = strstr(instr,search_str);
    if(!str)
    {
        len = 0;
        return NULL;
    }
    len = str - instr;
    memset(buf,0,sizeof(buf));
    memcpy(buf,instr,len);
    *buf_size = strlen(buf);
    //printf("buf:%s\n",buf);
    return (char *)buf;
}

int Rev_Parse(char * instring)
{
    TCP_Rev_parser_init(&Dst_address_info);
    
    Dst_info = &Dst_address_info;

    instr = instring;
    str = NULL;
    lenth = 0;

    if((!instr) || (!Dst_info)){
        ESP_LOGI("TCP_Rev_Parse", "err");
        return 0;
    }

    str = parse_elem(instr, ":", &lenth);
    if(str == NULL){
        return 0;
    }

    if(!strcmp(str, Dst)){
        instr = instr + lenth + strlen(":");
        str = parse_elem(instr, ".", &lenth);
        Dst_info->Dst_x = atol(str);
        printf("%d\r\n", Dst_info->Dst_x);
        instr = instr + lenth + strlen(".");
        Dst_info->Dst_y = atol(instr);
        printf("%d\r\n", Dst_info->Dst_y);
        printf("Dstaddress OK\n");
        xTaskNotify(Remote_Control_Task_Handle, BIT_4, eSetBits);
        return 1;
    }else if(!strcmp(str, E)){
        instr = instr + lenth + strlen(":");
        Dst_info->E_Distance = atol(instr);
        printf("%d\r\n", Dst_info->E_Distance);
        printf("right OK\n");
        xTaskNotify(Remote_Control_Task_Handle, BIT_0, eSetBits);
        return 1;
    }else if(!strcmp(str, S)){
        instr = instr + lenth + strlen(":");
        Dst_info->S_Distance = atol(instr);
        printf("%d\r\n", Dst_info->S_Distance);
        printf("behind OK\n");
        xTaskNotify(Remote_Control_Task_Handle, BIT_1, eSetBits);
        return 1;
    }else if(!strcmp(str, W)){
        instr = instr + lenth + strlen(":");
        Dst_info->W_Distance = atol(instr);
        printf("%d\r\n", Dst_info->W_Distance);
        printf("left OK\n");
        xTaskNotify(Remote_Control_Task_Handle, BIT_2, eSetBits);
        return 1;
    }else if(!strcmp(str, N)){
        instr = instr + lenth + strlen(":");
        Dst_info->N_Distance = atol(instr);
        printf("%d\r\n", Dst_info->N_Distance);
        printf("front OK\n");
        xTaskNotify(Remote_Control_Task_Handle, BIT_3, eSetBits);
        return 1;
    }
    
    printf("Invalid instruction\n");
    return 1;
}

void Remote_Control_Task(void *arg)
{
    BaseType_t xResult;
    uint32_t ulValue;
    while(1){
        xResult = xTaskNotifyWait(0x00000000,  0xFFFFFFFF, &ulValue, portMAX_DELAY);
        if(xResult == pdPASS){
            switch (ulValue){
            case BIT_0:
                    C610_SendCurrentVal(+Val, -Val, -Val, +Val);
                    // Sent_DstVal(50, Val, 50, 1, -1, -1, 1);
                    printf("right hello world\n");
                    break;  

            case BIT_1:
                    C610_SendCurrentVal(-Val, -Val, +Val, +Val);
                    // Sent_DstVal(50, Val, 50, -1, -1, 1, 1);
                    printf("behind hello world\n");
                    break;  

            case BIT_2:
                    C610_SendCurrentVal(-Val, +Val, +Val, -Val);
                    // Sent_DstVal(50, Val, 50, -1, +1, +1, -1);
                    printf("left hello world\n");
                    break;  

            case BIT_3:
                    C610_SendCurrentVal(+Val, +Val, -Val, -Val);
                    // Sent_DstVal(50, Val, 50, 1, 1, -1, -1);
                    printf("front hello world\n");
                    break;  

            case BIT_4:

                    printf("Dstaddress hello world\n");
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
    xReturn = xTaskCreate(Remote_Control_Task, "Remote_Control_Task", 1024*2, NULL, 5, &Remote_Control_Task_Handle);
    if(xReturn == pdPASS)
        printf("Remote_Control_Task create success!\r\n");
}

