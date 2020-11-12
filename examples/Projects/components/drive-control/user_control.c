#include "user_control.h"

const float ZERO	= 0.0000000f;
const float SQRT_3	= 1.7320508f;	//0x3fddb3d7
const float HALF_SQRT_2	= 0.70710678f;

/*һЩ��Ҫ������ֵ*/
const float Forest_x = -725.0f;
const float CTL_x = -1500.0f;

const float Velocity_y = 1000.0f;

volatile UnderpanMode_Typedef UnderpanMode = UnderpanMode_Standby;
volatile ESCMode_TypeDef ESCMode = ESCMode_Disable;

float Vel_Row, Vel_Col, Vel_Shoot, Vel_Curve;



/*���̲�������*/
UnderpanConfig_t UnderpanConfig = 
{
	1000.0f,		//MaxAxisSpeed
	5000.0f,		//MaxSpeed
	3000.0f,		//AxisAcc	2000 / 200 = 10
	3000.0f,		//OmegaAcc
	8.0f,			//OmegaRatio
	10.0f,			//SpeedSoftenValue
	5.0f,			//OmegaSoftenValue
	4000,
	SpeedSoften_OFF,
	ForwardAngle_0Deg,
	UnderpanMode_Stop_No_Servo
};

/*����״̬��ʼ��*/
Underpan_Status_t Underpan = 
{
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
};


/*���̿�������*/
void UnderpanControlTask(void)
{
	/*	����ң���Ҳ���������ѡ��ģʽ
	 *	��ע��
	 *		RF�źŶ�ʧ���Զ��л���״̬ SW0,��Ӧ Standby ģʽ,�����Զ�ʧ��
	 *		���ջ����ߵ����Ź�ʱ�䵽���, ����л��� Standby
	 *	P.S.
	 *		���Ź��Ѿ��Ƴ�
	 */
	switch((uint16_t)Remote_PulseToSwitch(Remote_GetT2()))
	{
		case Remote_SW0:
			UnderpanMode = UnderpanMode_Standby;
			break;
		case Remote_SW1:
			UnderpanMode = UnderpanMode_Remote;
			break;
		case Remote_SW2:
			UnderpanMode = UnderpanMode_Automic;
			break;
		default:
			if(UnderpanMode == UnderpanMode_Remote)
				UnderpanMode = UnderpanMode_Standby;
	}
	
    ESP_LOGI("Remote", "Remote_SWX: %d", (uint16_t)Remote_PulseToSwitch(Remote_GetT2()));

	/*	��ģʽ�µ�������	*/
	switch((uint8_t)UnderpanMode)
	{
		/*	Standby ģʽ, ESC�ض����,ȫ���ֿ��ֶ�ת��*/
		case UnderpanMode_Standby:
			Underpan_LostForce();
			break;
		
		/*	ң��ģʽ	*/
		case UnderpanMode_Remote:
			ESCMode = ESCMode_Velocity;
			UnderpanRemoteTask();
			break;
		
		/*	�Զ�ģʽ	*/
		case UnderpanMode_Automic:
			ESCMode = ESCMode_Velocity;
            Auto_Test_Task();
			break;
		
		/*	����ģʽ	*/
		case UnderpanMode_Debug:
			ESCMode = ESCMode_Velocity;
			UnderpanDebugTask();
			break;
	}
}

void Underpan_LostForce(void)
{
	ESCMode = ESCMode_Disable;
	
	PID_Increment_Clear(&PID_ESC_Velocity[0]);
	PID_Increment_Clear(&PID_ESC_Velocity[1]);
	PID_Increment_Clear(&PID_ESC_Velocity[2]);
	PID_Increment_Clear(&PID_ESC_Velocity[3]);
	
	Underpan.Speed_x = 0;
	Underpan.Speed_y = 0;
	Underpan.Speed_a = 0;
	
	Underpan.dstSpeed_x = 0;
	Underpan.dstSpeed_y = 0;
	Underpan.dstSpeed_a = 0;
	
	Underpan.Speed_Wheel_1 = 0;
	Underpan.Speed_Wheel_2 = 0;
	Underpan.Speed_Wheel_3 = 0;
	Underpan.Speed_Wheel_4 = 0;	

    // ESP_LOGI("Speed", "Speed_Wheel_1: %f, Speed_Wheel_2: %f, Speed_Wheel_3: %f, Speed_Wheel_4: %f", 
    //                         Underpan.Speed_Wheel_1, 
    //                         Underpan.Speed_Wheel_2, 
    //                         Underpan.Speed_Wheel_3, 
    //                         Underpan.Speed_Wheel_4);

}

void UnderpanStandbyTask(void)		//Standbyģʽ����
{
	Underpan.Speed_x = 0;
	Underpan.Speed_y = 0;
	Underpan.Speed_a = 0;
}

void UnderpanRemoteTask(void)
{
	/*
	 *	Ratio_S		�����ٶ���ҡ��ֵ�ı���ϵ��
	 *	Ratio_A		���̽��ٶ���ҡ��ֵ�ı���ϵ��
	 *	˵����
	 *			RM3508���ؼ���ת��ԼΪ8000, ��8192Ϊ��:
	 *		���پ�Ϊ��8192ʱ, ǰ����������ͶӰΪ8192 * Sqrt(2) = 11585rpm
	 *		�ۺ�ǰ���ٶ�Ϊ 4.80 m/s, ҡ������-683 ~ +683(��ֵ1024)
	 *		��Ratio_S = MaxAxisSpeed / 1000ʱ, ��MaxAxisSpeed = 10000,
	 *		����Ϊ����ٶȵ� 83%, 12000 ʱ������
	 */
	float Ratio_S = UnderpanConfig.MaxSpeed / 1000.0f;
	float Ratio_A = UnderpanConfig.MaxSpeed / 10000.0f;
	
	/*	ת��ͨ��ֵ	*/
	int16_t LX = Remote_PulseToVal(Remote_GetLY());
	int16_t RX = Remote_PulseToVal(Remote_GetRX());
	int16_t RY = Remote_PulseToVal(Remote_GetRY());
	
	/*	�ٶȷ���	*/
	float Vx, Vy, Va;

	
	switch((uint8_t)Remote_PulseToSwitch(Remote_GetT1()))
	{
		case Remote_SW0:	//�Գ�������ϵ����
			Vx = RX * Ratio_S;
			Vy = RY * Ratio_S;
			Va = LX * Ratio_A;
			break;
		case Remote_SW1:	//�Գ�������ϵ���ƣ�����
			Vx = -RX * Ratio_S;
			Vy = -RY * Ratio_S;
			Va = LX * Ratio_A;
			break;
		default:
			Vx = 0;
			Vy = 0;
			Va = 0;
	}
	
    // ESP_LOGI("Speed", "Vx: %f", Vx);
    // ESP_LOGI("Speed", "Vy: %f", Vy);
    // ESP_LOGI("Speed", "Va: %f", Va);
    

	/*	�ٶ��ữ	*/
	if(UnderpanConfig.SpeedSoften == SpeedSoften_ON)
	{
		Underpan.Speed_x = FlexibelValue(Vx, Underpan.Speed_x, 5000.0f / 200);
		Underpan.Speed_y = FlexibelValue(Vy, Underpan.Speed_y, 5000.0f / 200);
		Underpan.Speed_a = FlexibelValue(Va, Underpan.Speed_a, 5000.0f / 200);
	}
	else
	{
		Underpan.Speed_x = Vx;
		Underpan.Speed_y = Vy;
		Underpan.Speed_a = Va;
	}
	
	/*	��������	*/
	CalcWheelSpeed();
}

/*�ٶ��ữ*/
void SpeedSoften(void)
{
	if(UnderpanConfig.SpeedSoften == SpeedSoften_ON)
	{
        Underpan.Speed_x = FlexibelValue(Underpan.dstSpeed_x, Underpan.Speed_x, UnderpanConfig.AxisAcc / 200.0f);
        Underpan.Speed_y = FlexibelValue(Underpan.dstSpeed_y, Underpan.Speed_y, UnderpanConfig.AxisAcc / 200.0f);
        Underpan.Speed_a = FlexibelValue(Underpan.dstSpeed_a, Underpan.Speed_a, UnderpanConfig.OmegaSoftenValue);
	}
}

/*��ƽ���ٶȺͽ��ٶȼ��������ٶ�*/
void CalcWheelSpeed(void)
{
    Underpan.Speed_Wheel_1 = +Underpan.Speed_x - Underpan.Speed_y - UnderpanConfig.OmegaRatio * Underpan.Speed_a;
    Underpan.Speed_Wheel_2 = +Underpan.Speed_x + Underpan.Speed_y - UnderpanConfig.OmegaRatio * Underpan.Speed_a;
    Underpan.Speed_Wheel_3 = -Underpan.Speed_x + Underpan.Speed_y - UnderpanConfig.OmegaRatio * Underpan.Speed_a;
    Underpan.Speed_Wheel_4 = -Underpan.Speed_x - Underpan.Speed_y - UnderpanConfig.OmegaRatio * Underpan.Speed_a;

    ESP_LOGI("Speed", "Speed_Wheel_1: %f, Speed_Wheel_2: %f, Speed_Wheel_3: %f, Speed_Wheel_4: %f", 
                            Underpan.Speed_Wheel_1, 
                            Underpan.Speed_Wheel_2, 
                            Underpan.Speed_Wheel_3, 
                            Underpan.Speed_Wheel_4);
}


void Auto_Test_Task(void)
{
    ESP_LOGI(TAG, "Auto_Test_Task");
}

void UnderpanDebugTask(void)
{
    ESP_LOGI(TAG, "UnderpanDebugTask");
}
