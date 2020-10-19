#include "user_pwm.h"

//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH 500  //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 1500 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 90       //Maximum angle in degree upto which servo can rotate


void pwm_Init(void)
{
    printf("initializing mcpwm servo control gpio......\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, 2);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, 12);

    //initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm......\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50; //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;     //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;     //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); //Configure PWM0A & PWM0B with above settings

    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1300);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 550);
    vTaskDelay(20);
}

static void Put_down_Action1(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 510);
}

static void Put_down_Action2(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 550);
}

static void Put_down_Action3(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1300);
}

static void Pick_up_Action1(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 510);
}

static void Pick_up_Action2(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 900);
}

static void Pick_up_Action3(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1300);
}

void Put_down_Action(void)
{
    int step = 1;
    while(step){
        switch (step)
        {
        case 1:
            Put_down_Action1();
            step = 2;
            vTaskDelay(1500 / portTICK_RATE_MS);
            break;

        case 2:
            Put_down_Action2();
            step = 3;
            vTaskDelay(1000 / portTICK_RATE_MS);
            break;

        case 3:
            Put_down_Action3();
            step = 0;
            vTaskDelay(100 / portTICK_RATE_MS);
            break;

        default:
            step = 0;
            break;
        }
    }
}

void Pick_up_Action(void)
{
    int step = 1;
    while(step){
        switch (step)
        {
        case 1:
            Pick_up_Action1();
            printf("\npick up action 1\n");
            step = 2;
            vTaskDelay(1500 / portTICK_RATE_MS);
            break;

        case 2:
            Pick_up_Action2();
            printf("\npick up action 2\n");
            step = 3;
            vTaskDelay(1000 / portTICK_RATE_MS);
            break;

        case 3:
            Pick_up_Action3();
            printf("\npick up action 3\n");
            step = 0;
            vTaskDelay(100 / portTICK_RATE_MS);
            break;

        default:
            step = 0;
            break;
        }
    }
}

/**
 * @brief Configure MCPWM module
 */
void mcpwm_servo_control(void *arg)
{
    static int flag;
    int val;
    while (1){
        val = (uint16_t)Remote_PulseToSwitch_Two(Remote_GetVA());
        if (val != flag){
            if (val == Remote_0){
                Put_down_Action();
                flag = Remote_0;
            }else if (val == Remote_1){
                Pick_up_Action();
                flag = Remote_1;
            }
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void mcpwm_control(void)
{
    xTaskCreate(mcpwm_servo_control, "mcpwm_servo_control", 1024*1, NULL, 5, NULL);
}
