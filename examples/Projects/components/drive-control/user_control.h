#ifndef  _USER_CONTROL_H
#define _USER_CONTROL_H

#include "user_motor.h"
#include "user_remote.h"
#include "user_math.h"
#include "esp_log.h"

#define SQRT_2	1.414213562373f
#define TAG "Project"

typedef enum
{
	SpeedSoften_OFF	= 0x00,
	SpeedSoften_ON	= 0x01,
}SpeedSoften_TypeDef;

typedef enum
{
	ForwardAngle_0Deg		= 0x00,
	ForwardAngle_90Deg_CCW	= 0x01,
	ForwardAngle_90Deg_CW	= 0x02,
	ForwardAngle_180Deg		= 0x02,
}ForwardAngle_TypeDef;

typedef enum
{
	UnderpanMode_Stop_No_Servo	= 0x00,		//��������
	UnderpanMode_Stop_Vel_Servo	= 0x01,		//�ٶȻ�����
	UnderpanMode_Stop_Pos_Servo	= 0x02,		//λ�û�����
	
	UnderpanMode_Straight_Line		= 0x11,	//ֱ���н�
	UnderpanMode_Straight_45Deg		= 0x12,	//45�� + 90�������н�
	UnderpanMode_Straight_Route		= 0x13,	//�켣ǰ��	
}UnderpanMode_TypeDef;

typedef enum
{
	UnderpanMode_Standby	= 0x00,
	UnderpanMode_InTest		= 0x01,
	UnderpanMode_Remote		= 0x02,
	UnderpanMode_Automic	= 0x03,
	UnderpanMode_Debug		= 0x04,
}UnderpanMode_Typedef;

typedef enum
{
	AutomicState_Standby = 0x00,
	AutomicState_StartSigRecieved = 0x01,
	AutomicState_ToMainPoint_1 = 0x02,
	AutomicState_ToMainPoint_2 = 0x03,
	AutomicState_ToMainPoint_3 = 0x04,
	AutomicState_ToMainPoint_4 = 0x05,
}AutomicModeState_TypeDef;

typedef enum
{
	ESCMode_Disable = 0x00,
	ESCMode_Current = 0x01,
	ESCMode_Velocity = 0x02,
	ESCMOde_Position = 0x03,
}ESCMode_TypeDef;

typedef struct UnderpanConfig_t
{
	float MaxAxisSpeed;	//����������ٶ�
	float MaxSpeed;		//�����ٶ�
	float AxisAcc;	//�����������ٶ�
	float OmegaAcc;		//���ϼ��ٶ�
	float OmegaRatio;	//���ٶȽ������
	float SpeedSoftenValue;		//�ٶ��Ử����ֵ
	float OmegaSoftenValue;		//���ٶ��Ử����ֵ
	int16_t CurrentLimitation;	//����������������
	SpeedSoften_TypeDef SpeedSoften;	//�ٶ��ữ
	ForwardAngle_TypeDef ForwardAngle;	//���̳����
	UnderpanMode_TypeDef UnderpanMode;	//����ģʽ
}UnderpanConfig_t;

typedef struct Underpan_Status_t
{
	
	float Speed_x;
	float Speed_y;
	float Speed_a;
	float dstSpeed_x;
	float dstSpeed_y;
	float dstSpeed_a;
	float Speed_Wheel_1;
	float Speed_Wheel_2;
	float Speed_Wheel_3;
	float Speed_Wheel_4;
}Underpan_Status_t;

typedef struct UnderpanServoState_t
{
	uint8_t Done_x;
	uint8_t Done_y;
}UnderpanServoState_t;

/*	Extern Global Variables	*/
extern volatile UnderpanMode_Typedef UnderpanMode;
extern UnderpanConfig_t UnderpanConfig;
extern Underpan_Status_t Underpan;
extern volatile ESCMode_TypeDef ESCMode;
extern float Vel_Row, Vel_Col, Vel_Shoot, Vel_Curve;

/*	�ٶ�Ԥ����	*/
void Underpan_LostForce(void);
void SpeedSoften(void);
void CalcWheelSpeed(void);

/*���񲿷�*/
void UnderpanControlTask(void);
void UnderpanStandbyTask(void);
void UnderpanRemoteTask(void);
void Auto_Test_Task(void);
void UnderpanDebugTask(void);

/*�㷨����*/
float VelocityCalc(float CurrentVal, float DstVal);


#endif
