#include "user_TCP.h"

TaskHandle_t QRCode_Parse_Send_Handle = NULL;
TaskHandle_t TCP_Recieve_Task_Handle = NULL;

static const char *TAG = "TCP";
char rx_buffer[128];
char addr_str[128];
int addr_family;
int ip_protocol;
int sock;
struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6

char qrcode_invalid_buff[] = "\033[31minvalid QR code\033[0m\n";
char format_err[] = "\033[33merr format,please input ':' after an instruction\033[0m\n";
char invalid_command[] = "\033[33minvalid command\033[0m\n" ;
char other_err[] = "\033[33merr\033[0m\n";
char success[] = "\033[32mRecieve success!\033[0m\n";

char HELP_Info_buff[600];
int lenth = 0;

void TCP_Server_Init(void)
{
#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 destAddr;
    bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
    destAddr.sin6_family = AF_INET6;
    destAddr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket binded");

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occured during listen: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket listening");

    uint addrLen = sizeof(sourceAddr);
    sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
        // break;
    }
    ESP_LOGI(TAG, "Socket accepted");
}

void QRCode_Parse_Send(void * arg)
{
    char QR_Info_buff[512];
    int len;
    char e[] = "East", s[] = "South", w[] = "West", n[] = "North";
    while (1) {
        if(QR_Code_Parse_OK_Flag == 1){
            len   = sprintf(QR_Info_buff,      "name:      %d\n", info->name);
            len += sprintf(QR_Info_buff + len, "self_x:    %d\n", info->self_x);
            len += sprintf(QR_Info_buff + len, "self_y:    %d\n", info->self_y);
            len += sprintf(QR_Info_buff + len, "self_z:    %d\n", info->self_z);
            len += sprintf(QR_Info_buff + len, "\n");

            if(info->target_t.E_target_name != 0){
                len += sprintf(QR_Info_buff + len, "E_direction:      %s\n", e);
                len += sprintf(QR_Info_buff + len, "E_target_name:    %d\n", info->target_t.E_target_name);
                len += sprintf(QR_Info_buff + len, "E_distance:       %d\n", info->target_t.E_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }
            if(info->target_t.S_target_name != 0){
                len += sprintf(QR_Info_buff + len, "S_direction:      %s\n", s);
                len += sprintf(QR_Info_buff + len, "S_target_name:    %d\n", info->target_t.S_target_name);
                len += sprintf(QR_Info_buff + len, "S_distance:       %d\n", info->target_t.S_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }
            if(info->target_t.W_target_name != 0){
                len += sprintf(QR_Info_buff + len, "W_direction:      %s\n", w);
                len += sprintf(QR_Info_buff + len, "W_target_name:    %d\n", info->target_t.W_target_name);
                len += sprintf(QR_Info_buff + len, "W_distance:       %d\n", info->target_t.W_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }
            if(info->target_t.N_target_name != 0){
                len += sprintf(QR_Info_buff + len, "N_direction:      %s\n", n);
                len += sprintf(QR_Info_buff + len, "N_target_name:    %d\n", info->target_t.N_target_name);
                len += sprintf(QR_Info_buff + len, "N_distance:       %d\n", info->target_t.N_distance);
                len += sprintf(QR_Info_buff + len, "\n");
            }

            len += sprintf(QR_Info_buff + len, "\n\n");
            printf("string length:%d", len);
            if(len > 512){
                ESP_LOGE(TAG, "Error : ");
                break;
            }

            int err = send(sock, QR_Info_buff, len, 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                break;
            }
#if CONFIG_PC_CONTROL_OPERATION_MODE
            QR_Control();
#endif
            QR_Code_Parse_OK_Flag = 0;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void Parse_Send_Task_Create(void)
{
    BaseType_t xReturn = pdPASS;
    xReturn = xTaskCreate((TaskFunction_t)QRCode_Parse_Send, "QRCode_Parse_Send", 1024 * 2, NULL, 5, &QRCode_Parse_Send_Handle);
    if(xReturn == pdPASS)
        printf("QRCode_Parse_Send Creat success!\n\n");
}

void TCP_Recieve_Task(void * arg)
{
    int format_err_len = strlen(format_err);
    int success_len = strlen(success);
    int invalid_command_len = strlen(invalid_command);
    int other_err_len = strlen(other_err);
    
    char * instr;
    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        // Error occured during receiving
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        }
        // Connection closed
        else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed");
            break;
        }
        // Data received
        else {
            // Get the sender's ip address as string
            if (sourceAddr.sin6_family == PF_INET) {
                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
            } else if (sourceAddr.sin6_family == PF_INET6) {
                inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
            }

            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
            ESP_LOGI(TAG, "%s", rx_buffer);
/*****************************************************/
            instr = rx_buffer;
            int back = Rev_Parse(instr);
            switch (back)
            {
            case 0:
                send(sock, format_err, len, 0);
                printf("\nerr format,please input ':' after an instruction\n");
                break;

            case 1:
                send(sock, success, len, 0);
                printf("\nRecieved\n");
            break;

            case 2:
                send(sock, invalid_command, len, 0);
                printf("\nInvalid command\n");
            break;

            case 3:
                send(sock, other_err, len, 0);
                printf("\nerr\n");
            break;

            default:
                break;
            }
/*****************************************************/
            int err = send(sock, rx_buffer, len, 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                break;
            }
        }
    }

    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket");
        shutdown(sock, 0);
        close(sock);
    }
    vTaskDelete(NULL);
}

void TCP_Server_Task_Create(void)
{
    BaseType_t xReturn = pdPASS;

    xReturn = xTaskCreate((TaskFunction_t)TCP_Recieve_Task, "TCP_Recieve_Task", 1024*3, NULL, 5, &TCP_Recieve_Task_Handle);
    if(xReturn == pdPASS)
        printf("TCP_Recieve_Task Creat success!\n\n");
}

int help_menu(void)
{
    lenth   = sprintf(HELP_Info_buff,             "\nleft:x----------Control the car to move to the left for x seconds\n");
    lenth += sprintf(HELP_Info_buff + lenth, "right:x---------Control the car to move to the right for x seconds\n");
    lenth += sprintf(HELP_Info_buff + lenth, "front:x---------Control the car to move to the front for x seconds\n");
    lenth += sprintf(HELP_Info_buff + lenth, "behind:x--------Control the car to move to the behind for x seconds\n");
    lenth += sprintf(HELP_Info_buff + lenth, "Dstaddress:x.y--Control the car to move to the x.y coordinate relative to itself\n");
    lenth += sprintf(HELP_Info_buff + lenth, "pickup:---------Control the robotic arm to pick up materials\n");
    lenth += sprintf(HELP_Info_buff + lenth, "putdown:--------Control the robotic arm to lower the material\n\n");
    printf("string length:%d", lenth);
    int err = send(sock, HELP_Info_buff, lenth, 0);
    if (err < 0) {
      ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
    return 0;
    }
    return 1;
}

void QR_code_invaild_report(void)
{
    int buff_len = strlen(qrcode_invalid_buff);
    int err = send(sock, qrcode_invalid_buff, buff_len, 0);
    if (err < 0) {
      ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
    }
}


