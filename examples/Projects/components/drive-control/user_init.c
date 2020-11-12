#include "user_init.h"


void user_Init(void)
{
    can_Init();

    driver_Init();

    uart_Init();

    led_Init();
}

void driver_Init(void)
{
	C610_SetVelocityPIDRatio(0, 5.0f, 0.5f, 0.00f);
	C610_SetVelocityPIDRatio(1, 5.0f, 0.5f, 0.00f);
	C610_SetVelocityPIDRatio(2, 5.0f, 0.5f, 0.00f);
	C610_SetVelocityPIDRatio(3, 5.0f, 0.5f, 0.00f);
		
	C610_SetPositionPIDRatio(0, 1.0f, 0.02f, 0.00f);
	C610_SetPositionPIDRatio(1, 1.0f, 0.02f, 0.00f);
	C610_SetPositionPIDRatio(2, 1.0f, 0.02f, 0.00f);
	C610_SetPositionPIDRatio(3, 1.0f, 0.02f, 0.00f);

    ESP_LOGI(TAG, "drive init is success");
}

void NVS_Flash_Init(void)
{
     /**< Initialize NVS */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

void led_Init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}


