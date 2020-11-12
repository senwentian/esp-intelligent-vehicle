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

    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 500);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 500);
    vTaskDelay(20);
}

/**
 * @brief Use this function to calcute pulse width for per degree rotation
 *
 * @param  degree_of_rotation the angle in degree to which servo has to rotate
 *
 * @return
 *     - calculated pulse width
 */
static uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}

static void Put_down_Action1(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1500);
}

static void Put_down_Action2(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 1500);
}

static void Put_down_Action3(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 500);
}

static void Pick_up_Action1(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 1500);
}

static void Pick_up_Action2(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 500);
}

static void Pick_up_Action3(void)
{
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 500);
}

void Pick_up_Action(void)
{
    int step = 1;
    while(step){
        switch (step)
        {
        case 1:
            Put_down_Action1();
            step = 2;
            break;

        case 2:
            Put_down_Action2();
            step = 3;
            break;

        case 3:
            Put_down_Action3();
            step = 0;
            break;

        default:
            step = 1;
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void Put_down_Action(void)
{
    int step = 1;
    while(step){
        switch (step)
        {
        case 1:
            Pick_up_Action1();
            step = 2;
            break;

        case 2:
            Pick_up_Action2();
            step = 3;
            break;

        case 3:
            Pick_up_Action3();
            step = 0;
            break;

        default:
            step = 1;
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

/**
 * @brief Configure MCPWM module
 */
void mcpwm_servo_control(void *arg)
{
    // /******************Test******************/
    // int flag = 1;
    // uint32_t angle, count;

    // while (1) {
    //     if(flag == 1){
    //         for (count = 0; count < SERVO_MAX_DEGREE; count+=6) {
    //             // printf("Angle of rotation: %d\n", count);
    //             angle = servo_per_degree_init(count);
    //             // printf("pulse width: %dus\n", angle);
    //             mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
    //             mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, angle);
    //             vTaskDelay(10);     //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation at 5V
    //         }
    //         flag = 0;
    //     }else{
    //         for (count = SERVO_MAX_DEGREE; count > 0; count-=6) {
    //             // printf("Angle of rotation: %d\n", count);
    //             angle = servo_per_degree_init(count);
    //             // printf("pulse width: %dus\n", angle);
    //             mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
    //             mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, angle);
    //             vTaskDelay(10);     //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation at 5V
    //         }
    //         flag = 1;
    //     }
    // }
    static int flag;
    int val;
    while (1)
    {
        val = (uint16_t)Remote_PulseToSwitch_Two(Remote_GetVA());
        if (val != flag)
        {
            if (val == Remote_0)
            {
                Put_down_Action();
                flag = 0x00;
            }
            else if (val == Remote_1)
            {
                Pick_up_Action();
                flag = 0x01;
            }
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void mcpwm_control(void)
{
    xTaskCreate(mcpwm_servo_control, "mcpwm_servo_control", 1024*2, NULL, 5, NULL);
}
