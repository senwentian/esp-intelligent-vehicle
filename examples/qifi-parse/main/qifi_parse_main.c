/* qifi parse Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "qifi_parser.h"
#include "esp_err.h"
#include "esp_log.h"

const char *string[] = {
"WIFI:T:WPA;S:mynetwork;P:mypass;;",
"WIFI:T:WEP;S:mynetwork;P:mypass;H:true;",
"WIFI:T:nopass;S:12345678901234567890123456789012;P:1234567890123456789012345678901234567890123456789012345678901234;H:true;",
"WIFI:T:omit;S:中文测试;P:中文密码;H:false;",
"WIFI:T:omit;S:S:\\;\\,\\:\\\\end;P:中文密码;H:false;",
// "WIFI:T:WPA;S:123456789012345678901234567890123;P:12345678901234567890123456789012345678901234567890123456789012345;H:false;",
};

static const char *TAG = "main";

void app_main()
{
    qifi_parser_t parser;
    esp_err_t ret = ESP_OK;
    uint32_t test_counts = sizeof(string) / sizeof(string[0]);

    for (int i = 0; i < test_counts; ++i) {
        qifi_parser_init(&parser);
        ESP_LOGI(TAG, "Start Parse String[%d]: %s", i, string[i]);
        ret = qifi_parser_parse(&parser, string[i], strlen(string[i]));
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "String[%d] Parse OK!\n------\nTYPE:%d\nSSID(%d):%.*s\nPASSWORD(%d):%.*s\nHIDDEN:%d\n------",
                i, parser.type, parser.ssid_len, parser.ssid_len, parser.ssid,
                parser.password_len, parser.password_len, parser.password, parser.ssid_hidden);
        } else {
            ESP_LOGE(TAG, "ret:0x%x", ret);
        }
    }
    ESP_LOGI(TAG, "TEST DONE");
}
