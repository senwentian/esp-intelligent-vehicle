#include "user_motor.h"
#include "driver/can.h"

/*Variable Includes Motor Informathin. Externed in user_motor.h*/
MotoInfo_t MotorInfo[4] = {0};
PID_Increment_t PID_ESC_Velocity[4], PID_ESC_Vel_Global;
PID_Regular_t	PID_ESC_Position[4], PID_ESC_Pos_Global;


/**
  * @brief  get the feedback information from ESC on can bus.
  * @param  RxMsg: pointer to a structure receive frame which contains can Id,
  *         can DLC, can data and FMI number.
  * @retval ESC ID (if not zero)
  */
void C610_GetMotorInfo(can_message_t* RxMsg)
{
	uint8_t ESC_Id = RxMsg ->identifier - ESC_BaseID;
	uint16_t LastPulse;
	int16_t DeltaPulse;
	if(ValueInRange_u(ESC_Id, 0, 3))
	{
		LastPulse = MotorInfo[ESC_Id].AnglePulse;
		MotorInfo[ESC_Id].AnglePulse = (uint16_t)(RxMsg -> data[0] << 8 | RxMsg -> data[1]);
		MotorInfo[ESC_Id].Velocity	 = (int16_t)(RxMsg -> data[2] << 8 | RxMsg -> data[3]);
		MotorInfo[ESC_Id].Current	 = (int16_t)(RxMsg -> data[4] << 8 | RxMsg -> data[5]);
		MotorInfo[ESC_Id].Temprature = (uint8_t)(RxMsg -> data[6]);
		DeltaPulse = MotorInfo[ESC_Id].AnglePulse - LastPulse;
		if(DeltaPulse > 4095)
			DeltaPulse -= 8192;
		else if(DeltaPulse < -4096)
			DeltaPulse += 8192;
		MotorInfo[ESC_Id].Position += DeltaPulse;
	}
	ESP_LOGI("MotorInfo", "AnglePulse: %d, Velocity: %d, Current: %d, Temprature: %d", 
							MotorInfo[ESC_Id].AnglePulse, 
							MotorInfo[ESC_Id].Velocity, 
							MotorInfo[ESC_Id].Current, 
							MotorInfo[ESC_Id].Temprature);
}

/**
  * @brief  Checks the current value(-32768 ~ +32767).
  * @param  None.
  * @retval  None
  */
void C610_SendCurrentVal(int16_t I1, int16_t I2, int16_t I3, int16_t I4)
{
	int16_t Current_1 = I1;
	int16_t Current_2 = I2;
	int16_t Current_3 = I3;
	int16_t Current_4 = I4;
	can_message_t message;
	message.flags = CAN_MSG_FLAG_NONE;
	message.identifier = 0x200;
  	message.data_length_code = 0x08;
    message.data[0] = (Current_1 >> 8);
    message.data[1] = Current_1;
    message.data[2] = (Current_2 >> 8);
    message.data[3] = Current_2;
    message.data[4] = (Current_3 >> 8);
    message.data[5] = Current_3;
    message.data[6] = (Current_4 >> 8);
    message.data[7] = Current_4;
	can_transmit(&message, portMAX_DELAY);
}

/**
  * @brief  Set Velocity Loop PID Ratio(Kp, Ki, Kd only).
  * @param  ESC_Id: ESC Id(0 .. 7).
  *			Kp, Ki, Kd: PID Ratio.
  * @retval None.
  */
void C610_SetVelocityPIDRatio(uint8_t ESC_Id, float Kp, float Ki, float Kd)
{
	PID_ESC_Velocity[ESC_Id].Kp = Kp;
	PID_ESC_Velocity[ESC_Id].Ki = Ki;
	PID_ESC_Velocity[ESC_Id].Kd = Kd;
	
	PID_ESC_Velocity[ESC_Id].Deadband = 50.0f;
	PID_ESC_Velocity[ESC_Id].MaxIncrease = 600.0f;
	PID_ESC_Velocity[ESC_Id].MaxInt = 3000.0f;
	PID_ESC_Velocity[ESC_Id].MaxOutput = 5000.0f;
}

void C610_SetPositionPIDRatio(uint8_t ESC_Id, float Kp, float Ki, float Kd)
{
	PID_ESC_Position[ESC_Id].Kp = Kp;
	PID_ESC_Position[ESC_Id].Ki = Ki;
	PID_ESC_Position[ESC_Id].Kd = Kd;
}

void C610_VelocityControl(uint8_t ESC_Id, float DstVelocity)
{
	PID_ESC_Velocity[ESC_Id].Feedback = MotorInfo[ESC_Id].Velocity;
	ESP_LOGI("Vel", "MotorInfo[].Velocity: %d", MotorInfo[ESC_Id].Velocity);
	PID_ESC_Velocity[ESC_Id].Ref = DstVelocity;
	PID_Increment_Calc(&PID_ESC_Velocity[ESC_Id]);
}

void C610_PositionControl(uint8_t ESC_Id, int32_t DstPosition)
{
	PID_ESC_Position[ESC_Id].Feedback = MotorInfo[ESC_Id].Position;
	PID_ESC_Position[ESC_Id].Ref = DstPosition;
	PID_Regular_Cacl(&PID_ESC_Position[ESC_Id]);
	C610_VelocityControl(ESC_Id, PID_ESC_Position[ESC_Id].Output);
}
