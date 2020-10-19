#include "user_uart.h"

QueueHandle_t uart1_queue;

void uart_Init(void)
{
     const uart_config_t uart_config = {
        .baud_rate = 100000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
     //Install UART driver, and get the queue.
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 4, 0, 20, &uart1_queue, 0);
    
    ESP_LOGI("Project", "UART Init is success");
}

void uart_recieve_task(void * arg)
{
    int read_len;
    uart_event_t event;
    ESP_LOGI("UART", "Hello_world");
    uint8_t* dtmp = (uint8_t*) malloc(1024);
    for(;;) {
        uint8_t flag = 1;
        //Waiting for UART event.
        xQueueReceive(uart1_queue, (void * )&event, (portTickType)portMAX_DELAY);
        bzero(dtmp, 1024);
        //ESP_LOGI("UART", "uart[%d] event:", UART_NUM_1);
        switch(event.type) {
            //Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                read_len = uart_read_bytes(UART_NUM_1, dtmp, event.size, portMAX_DELAY);
                //ESP_LOGI("UART", "Read %d bytes: %d, %d, %d, %d, %d", read_len, *dtmp, *(dtmp+1), *(dtmp+2), *(dtmp+3), *(dtmp+4));
                dtmp[read_len] = 0;
                const uint8_t *d = dtmp;
                static uint8_t RxState = 0, RxDataIndex = 0;
                while(flag){
                    switch(RxState){
                        case 0:				//The start mark has not been detected yet, start to match the start mark
                            if(*d == 0x0F)
                            {
                                RxState++;
                                SBUS_MsgPack[0] = *d;
                                // ESP_LOGI("UART", "SBUS_Msg[0]: %d",SBUS_MsgPack[0]);
                                RxDataIndex = 1;
                            }
                            else
                                RxState = 0;
                            break;
                        case 1:					//Start mark matches, start to receive raw data
                            SBUS_MsgPack[RxDataIndex] = *d;
                            // ESP_LOGI("UART", "SBUS_Msg[%d]: %d",RxDataIndex, SBUS_MsgPack[RxDataIndex]);
                            RxDataIndex++;
                            if(RxDataIndex >= 23)
                            {
                                RxDataIndex = 0;
                                RxState++;
                            }
                            break;
                        case 2:					//The data has been received, the first byte of the start matching end flag
                                SBUS_MsgPack[23] = *d;
                                //ESP_LOGI("UART", "SBUS_Msg[23]: %d",SBUS_MsgPack[23]);
                                RxState++;
                            break;
                        case 3:					//The first byte of the end flag is matched, and the second byte of the start matching end flag
                            if(*d == 0x00)
                            {
                                UpdateRemoteInfo((void*)&SBUS_MsgPack[0]);
                                SBUS_MsgPack[24] = 0x00;
                                //ESP_LOGI("UART", "SBUS_Msg[24]: %d",SBUS_MsgPack[24]);
                                RemoteUpdated = 1;
                            }
                            RxState = 0;
                            flag = 0;
                            break;
                        default:
                            RxState = 0;
                    }
                    d++;
                }
                // ESP_LOGI("UART", "Remote.Rx: %d, %d, %d, %d, %d, %d", SBUS_ChanelVal[0], SBUS_ChanelVal[1], SBUS_ChanelVal[2], SBUS_ChanelVal[4], SBUS_ChanelVal[5], SBUS_ChanelVal[6]);
                break;

            default:
                // ESP_LOGI("UART", "uart event type: %d", event.type);
                break;
            }
    }
    free(dtmp);
    dtmp = NULL;
}

void UART_Recieve_Task_Create(void)
{
    BaseType_t xReturn = pdPASS;

    xReturn = xTaskCreate((TaskFunction_t)uart_recieve_task, "uart_recieve_task", 1024 * 2, NULL, 5, NULL);
    if(xReturn == pdPASS)
        printf("uart_recieve_task Creat success!\n\n");
}