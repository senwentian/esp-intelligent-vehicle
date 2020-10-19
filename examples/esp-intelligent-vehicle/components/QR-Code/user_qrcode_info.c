/* quirc -- QR-code recognition library
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qifi_parser.h"
#include "user_qrcode_wifi_task.h"
#include "quirc.h"
#include "esp_log.h"
#include "user_camera.h"
#include "user_camera.h"
#include "user_TCP.h"
#include "user_qrcode_info.h"
#include "user_qrcode_task.h"

#define APP_QIFI_PRINT_QR               CONFIG_APP_QIFI_PRINT_QR

QR_Code_Info_t QR_dode_info;
QR_Code_Info_t * info;
uint8_t direction_num;

static char * instr;
static char * str;
static int len;

static const char *TAG = "qrcode-infomation";

static void app_dump_cells(const struct quirc_code *code)
{
    int u = 0, v = 0;

    printf("    %d cells, corners:", code->size);

    for (u = 0; u < 4; u++) {
        printf(" (%d,%d)", code->corners[u].x, code->corners[u].y);
    }

    printf("\n");

    for (v = 0; v < code->size; v++) {
        printf("\033[0m    ");

        for (u = 0; u < code->size; u++) {
            int p = v * code->size + u;

            if (code->cell_bitmap[p >> 3] & (1 << (p & 7))) {
                printf("\033[40m  ");
            } else {
                printf("\033[47m  ");
            }
        }

        printf("\033[0m\n");
    }
}

void QR_code_parser_init(QR_Code_Info_t *parser)
{
    if (parser) {
        memset(parser, 0x0, sizeof(QR_Code_Info_t));
    }
    return;
}

char *parse_elem(char * instr,char *search_str,int *buf_size)
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

static void QR_code_data(const struct quirc_data *data)
{
    printf("\033[31m    Payload: %s\n", data->payload);

    QR_code_parser_init(&QR_dode_info);
    direction_num = 0;

    info = &QR_dode_info;

    instr = data->payload;
    str = NULL;
    // string = NULL;
    len = 0;

    if((!instr) || (!info)){
        ESP_LOGI("QR_code_data", "err");
    }

    str = parse_elem(instr, "/", &len);
    info->name = atol(str);
    instr = instr + len + strlen("/");
    // printf("%d\r\n", info->name);

    str = parse_elem(instr, "/", &len);
    info->self_x = atol(str);
    instr = instr + len + strlen("/");
    // printf("%d\r\n", info->self_x);

    str = parse_elem(instr, "/", &len);
    info->self_y = atol(str);
    instr = instr + len + strlen("/");
    // printf("%d\r\n", info->self_y);

    str = parse_elem(instr, "/", &len);
    info->self_z = atol(str);
    instr = instr + len + strlen("/");
    // printf("%d\r\n", info->self_z);

    while (instr){
        str = parse_elem(instr, ":", &len);
        switch (*str){
        case 'e':
            info->target_t.E_direction = *str;
            instr = instr + len + strlen(":");
            // printf("%c\r\n", info->target_t.E_direction);

            str = parse_elem(instr, ":", &len);
            info->target_t.E_target_name = atol(str);
            instr = instr + len + strlen(":");
            // printf("%d\r\n", info->target_t.E_target_name);
            direction_num += right_num;
            str = parse_elem(instr, "/", &len);
            if(!str){
                info->target_t.E_distance = atol(instr);
                // printf("%d\r\n", info->target_t.E_distance);
                instr = NULL;
            }else{
                info->target_t.E_distance = atol(str);
                instr = instr + len + strlen("/");
                // printf("%d\r\n", info->target_t.E_distance);
            }
            break;

        case 's':
            info->target_t.S_direction = *str;
            instr = instr + len + strlen(":");
            // printf("%c\r\n", info->target_t.S_direction);

            str = parse_elem(instr, ":", &len);
            info->target_t.S_target_name = atol(str);
            instr = instr + len + strlen(":");
            // printf("%d\r\n", info->target_t.S_target_name);
            direction_num += behind_num;
            str = parse_elem(instr, "/", &len);
            if(!str){
                info->target_t.S_distance = atol(instr);
                // printf("%d\r\n", info->target_t.S_distance);
                instr = NULL;
            }else{
                info->target_t.S_distance = atol(str);
                instr = instr + len + strlen("/");
                // printf("%d\r\n", info->target_t.S_distance);
            }
            break;

        case 'w':
            info->target_t.W_direction = *str;
            instr = instr + len + strlen(":");
            // printf("%c\r\n", info->target_t.W_direction);

            str = parse_elem(instr, ":", &len);
            info->target_t.W_target_name = atol(str);
            instr = instr + len + strlen(":");
            // printf("%d\r\n", info->target_t.W_target_name);
            direction_num += left_num;
            str = parse_elem(instr, "/", &len);
            if(!str){
                info->target_t.W_distance = atol(instr);
                // printf("%d\r\n", info->target_t.W_distance);
                instr = NULL;
            }else{
                info->target_t.W_distance = atol(str);
                instr = instr + len + strlen("/");
                // printf("%d\r\n", info->target_t.W_distance);
            }
            break;

        case 'n':
            info->target_t.N_direction = *str;
            instr = instr + len + strlen(":");
            // printf("%c\r\n", info->target_t.N_direction);

            str = parse_elem(instr, ":", &len);
            info->target_t.N_target_name = atol(str);
            instr = instr + len + strlen(":");
            // printf("%d\r\n", info->target_t.N_target_name);
            direction_num += front_num;
            str = parse_elem(instr, "/", &len);
            if(!str){
                info->target_t.N_distance = atol(instr);
                // printf("%d\r\n", info->target_t.N_distance);
                instr = NULL;
            }else{
                info->target_t.N_distance = atol(str);
                instr = instr + len + strlen("/");
                // printf("%d\r\n", info->target_t.N_distance);
            }
            break;

        default:
            break;
        }
    }
    app_set_QR_code_state(QR_QIFI_STRING_PARSE_OK);
}

static void QR_code_dump_info(struct quirc *q, uint8_t count)
{
    printf("%d QR-codes found:\n\n", count);

    for (int i = 0; i < count; i++) {
        struct quirc_code code;
        struct quirc_data data;

        // Extract the QR-code specified by the given index.
        quirc_extract(q, i, &code);

        //Decode a QR-code, returning the payload data.
        quirc_decode_error_t err = quirc_decode(&code, &data);
        
#if APP_QIFI_PRINT_QR
        app_dump_cells(&code);
        printf("\n");
#endif

        if (err) {
            printf("  Decoding FAILED: %s\n", quirc_strerror(err));
        } else {
            printf("  Decoding successful:\n");
            printf("    %d cells, corners:", code.size);

            for (uint8_t u = 0; u < 4; u++) {
                printf(" (%d,%d)", code.corners[u].x, code.corners[u].y);
            }

            printf("\n");
            QR_code_data(&data);
        }
        printf("\n");
    }
}

esp_err_t QR_code_info_scan(struct quirc *qr_recognizer)
{
    int id_count = 0;
    camera_fb_t *fb = NULL;
    uint8_t *image = NULL;
    // Save image width and height, avoid allocate memory repeatly.
    static uint16_t old_width = 0;
    static uint16_t old_height = 0;

    // Capture a frame
    fb = esp_camera_fb_get();

    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return ESP_FAIL;
    } 

    if (old_width != fb->width || old_height != fb->height) {
        ESP_LOGI(TAG, "Recognizer(%p) size change w h len: %d, %d, %d", qr_recognizer, fb->width, fb->height, fb->len);

        // Resize the QR-code recognizer.
        if (quirc_resize(qr_recognizer, fb->width, fb->height) < 0) {
            ESP_LOGE(TAG, "Resize the QR-code recognizer err.");
            return ESP_FAIL;
        } else {
            old_width = fb->width;
            old_height = fb->height;
        }
    }

    image = quirc_begin(qr_recognizer, NULL, NULL);
    memcpy(image, fb->buf, fb->len);
    quirc_end(qr_recognizer);

    // Return the number of QR-codes identified in the last processed image.
    id_count = quirc_count(qr_recognizer);

    if (id_count == 0) {
        ESP_LOGW(TAG, "invalid QR code");
        QR_code_invaild_report();
        esp_camera_fb_return(fb);
        return ESP_FAIL;
    }

    // Print information of QR-code
    QR_code_dump_info(qr_recognizer, id_count);
    esp_camera_fb_return(fb);
    return ESP_OK;
}

