#include "user_init.h"

volatile int OTA_flag = 0;

static void button_press_3sec_cb(void *arg)
{
    ESP_LOGW("Project", "Restore factory settings");
    nvs_flash_erase();
    esp_restart();
}

static void button_press_cb(void *arg)
{
    ESP_LOGW("Project", "Start firmware upgrade");
    OTA_flag = 1;
}

static void configure_push_button(int gpio_num, void (*btn_cb)(void *))
{
    button_handle_t btn_handle = iot_button_create(gpio_num, 0);

    if (btn_handle) {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, button_press_cb, NULL);
        iot_button_add_on_press_cb(btn_handle, 3, button_press_3sec_cb, NULL);
    }
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

    ESP_LOGI("Project", "drive init is success");
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



void user_Init(void)
{
    can_Init();

    // driver_Init();
    // configure_push_button(GPIO_NUM_0, NULL);

    uart_Init();

    // led_Init();

    pwm_Init();
}

void button_Init(void)
{
    configure_push_button(GPIO_NUM_0, NULL);
}
