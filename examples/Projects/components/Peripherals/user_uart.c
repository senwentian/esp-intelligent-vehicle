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
    
    ESP_LOGI(TAG, "UART Init is success");
}
