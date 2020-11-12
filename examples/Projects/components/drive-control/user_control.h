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
	UnderpanMode_Stop_No_Servo	= 0x00,		//开环抱死
	UnderpanMode_Stop_Vel_Servo	= 0x01,		//速度环抱死
	UnderpanMode_Stop_Pos_Servo	= 0x02,		//位置环抱死
	
	UnderpanMode_Straight_Line		= 0x11,	//直线行进
	UnderpanMode_Straight_45Deg		= 0x12,	//45° + 90°折线行进
	UnderpanMode_Straight_Route		= 0x13,	//轨迹前进	
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
	float MaxAxisSpeed;	//最大正交轴速度
	float MaxSpeed;		//最大合速度
	float AxisAcc;	//最大正交轴加速度
	float OmegaAcc;		//最大合加速度
	float OmegaRatio;	//角速度解算比例
	float SpeedSoftenValue;		//速度柔滑步进值
	float OmegaSoftenValue;		//角速度柔滑步进值
	int16_t CurrentLimitation;	//驱动器最大电流限制
	SpeedSoften_TypeDef SpeedSoften;	//速度柔化
	ForwardAngle_TypeDef ForwardAngle;	//底盘朝向角
	UnderpanMode_TypeDef UnderpanMode;	//底盘模式
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

/*	速度预处理	*/
void Underpan_LostForce(void);
void SpeedSoften(void);
void CalcWheelSpeed(void);

/*任务部分*/
void UnderpanControlTask(void);
void UnderpanStandbyTask(void);
void UnderpanRemoteTask(void);
void Auto_Test_Task(void);
void UnderpanDebugTask(void);

/*算法部分*/
float VelocityCalc(float CurrentVal, float DstVal);


#endif
