/* ESP Put Picture to COS Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "app_wifi.h"

#include "esp_http_client.h"

static const char *TAG = "ppc";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
            }
            printf("%.*s\r\n", evt->data_len, (char*)evt->data);

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

static void http_rest_with_url()
{
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    /**
     * A typical HTTP PUT format: (new line with \r\n)
     * 
     * PUT /ESP32%2F2021%2F2020.txt HTTP/1.1
     * Host: espskr-server-1300057161.cos.ap-shanghai-fsi.myqcloud.com
     * User-Agent:User-Agent: cos-sdk-c/5.0.5(Compatible Unknown)
     * Accept: *//*
     * Content-Length: 8
     * Content-Type: text/plain
     * Content-MD5: OctRfXdSCt5FmGdIpEbMLQ==
     * x-cos-security-token: MyTokenString
     * Date: Mon, 28 Oct 2019 03:31:21 GMT
     * Authorization: q-sign-algorithm=sha1&q-ak=AKIDBF9qMiZ42wjNp7PF5iAXIuLdjaEcnlUz&q-sign-time=1572233481;1572233781&q-key-time=1572233481;1572233781&q-header-list=host&q-url-param-list=&q-signature=7f459ada58b1a6c317b0f86b987940c6df57aba5
     * 
     * HTTP body
     * 
    */

    //PUT
    esp_http_client_set_url(client, "http://espskr-server-1300057161.cos.ap-shanghai-fsi.myqcloud.com/1991/1991.md");
    const char *post_data = "112233\r\n";
    esp_http_client_set_method(client, HTTP_METHOD_PUT);
    // User-Agent:User-Agent: cos-sdk-c/5.0.5(Compatible Unknown)
    // Host: espskr-server-1300057161.cos.ap-shanghai-fsi.myqcloud.com
    // Content-Type: text/plain
    // x-cos-security-token: MyTokenString

    esp_http_client_set_header(client, "User-Agent", "cos-sdk-c/5.0.5(Compatible Unknown)");
    esp_http_client_set_header(client, "Host", "espskr-server-1300057161.cos.ap-shanghai-fsi.myqcloud.com");
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_http_client_set_header(client, "x-cos-security-token", "MyTokenString");

    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP PUT Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

static void put_picture_to_cos(void *pvParameters)
{
    app_wifi_wait_connected();
    ESP_LOGI(TAG, "Connected to AP, begin http example");

    http_rest_with_url();

    ESP_LOGI(TAG, "Finish http example");
    vTaskDelete(NULL);
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    app_wifi_initialise();

    xTaskCreate(&put_picture_to_cos, "ppc", 8192, NULL, 5, NULL);
}
