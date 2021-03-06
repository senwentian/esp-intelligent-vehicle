#ifndef  _USER_CONTROL_H
#define _USER_CONTROL_H

#include "user_motor.h"
#include "user_remote.h"
#include "user_math.h"
#include "esp_log.h"

#define SQRT_2	1.414213562373f

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
	UnderpanMode_Stop_No_Servo	= 0x00,		//Open loop
	UnderpanMode_Stop_Vel_Servo	= 0x01,		//Speed loop
	UnderpanMode_Stop_Pos_Servo	= 0x02,		//Position loop
	
	UnderpanMode_Straight_Line		= 0x11,	//Straight line
	UnderpanMode_Straight_45Deg		= 0x12,	//45° + 90° polyline travel
	UnderpanMode_Straight_Route		= 0x13,	//Trajectory	
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
	float MaxAxisSpeed;	//Maximum orthogonal axis speed
	float MaxSpeed;		//Maximum closing speed
	float AxisAcc;	//Maximum orthogonal axis acceleration
	float OmegaAcc;		//Maximum closing acceleration
	float OmegaRatio;	//Angular velocity solution ratio
	float SpeedSoftenValue;		//Speed silky step value
	float OmegaSoftenValue;		//Angular velocity silky step value
	int16_t CurrentLimitation;	//Maximum drive current limit
	SpeedSoften_TypeDef SpeedSoften;	//Speed softening
	ForwardAngle_TypeDef ForwardAngle;	//Chassis angle
	UnderpanMode_TypeDef UnderpanMode;	//Chassis mode
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

/*Speed preprocessing*/
void Underpan_LostForce(void);
void SpeedSoften(void);
void CalcWheelSpeed(void);

/*Task Part*/
void UnderpanControlTask(void);
void UnderpanStandbyTask(void);
void UnderpanRemoteTask(void);
void Auto_Test_Task(void);
void UnderpanDebugTask(void);


float VelocityCalc(float CurrentVal, float DstVal);


#endif
