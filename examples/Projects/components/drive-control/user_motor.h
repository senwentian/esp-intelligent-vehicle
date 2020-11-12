#ifndef  _USER_MOTOR_H
#define _USER_MOTOR_H

#include "esp_log.h"

#include "driver/can.h"

#include "user_math.h"
#include "user_pid.h"
#include "user_control.h"

#define ESC_BaseID	0x201

#define	ID_ESC1	0x201
#define	ID_ESC2	0x202
#define	ID_ESC3	0x203
#define	ID_ESC4	0x204

typedef struct MotoInfo_t
{
	uint16_t	AnglePulse;
	int16_t		Velocity;
	int16_t		Current;
	uint8_t		Temprature;
	int32_t		Position;
}MotoInfo_t;

extern MotoInfo_t MotorInfo[];
extern PID_Increment_t PID_ESC_Vel_Global, PID_ESC_Velocity[];
extern PID_Regular_t PID_ESC_Pos_Global;


/*  Function used to get the feedback information from ESC on can bus *********/
 void C610_GetMotorInfo(can_message_t* RxMsg);

/*  Function used to send the current value to ESC to control the motor *******/
void C610_SendCurrentVal(int16_t I1, int16_t I2, int16_t I3, int16_t I4);

void C610_VelocityControl(uint8_t ESC_Id, float DstVelocity);
void C610_SetVelocityPIDRatio(uint8_t ESC_Id, float Kp, float Ki, float Kd);
void C610_SetPositionPIDRatio(uint8_t ESC_Id, float Kp, float Ki, float Kd);
void C610_VelocityControl(uint8_t ESC_Id, float DstVelocity);
void C610_PositionControl(uint8_t ESC_Id, int32_t DstPosition);





#endif
