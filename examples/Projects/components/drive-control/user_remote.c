#include "user_remote.h"

volatile uint8_t RemoteUpdated = 0;
Remote_t Remote;

void RemoteInit(void)
{
	Remote.LX = 0;
	Remote.LY = 0;
	Remote.RX = 0;
	Remote.RY = 0;
	Remote.T1 = 0;
	Remote.S1 = 0;
	Remote.VA = 0;
	Remote.S2 = 0;
	Remote.T2 = 0;
}

void UpdateRemoteInfo(void* PWM_Array)
{
	/*Prevent Warning*/
	(void)PWM_Array;
	
	SBUS_Decode();
	Remote.RX = SBUS_ChanelVal[0];
	Remote.RY = SBUS_ChanelVal[1];
	Remote.LY = SBUS_ChanelVal[2];
	Remote.LX = SBUS_ChanelVal[3];
	Remote. T2 = SBUS_ChanelVal[4];
	Remote.T1 = SBUS_ChanelVal[5];
	Remote.VA = SBUS_ChanelVal[6];
	Remote.S2 = SBUS_ChanelVal[7];
	Remote.S1 = SBUS_ChanelVal[8];
}

uint16_t Remote_GetChanalVal(uint8_t Chanel)
{
	if((Chanel > 0) && (Chanel < 10))
		return SBUS_ChanelVal[Chanel - 1];
	return 0;
}

int16_t Remote_GetLX(void)
{
	return *((volatile int16_t*)&Remote.LX);
}

int16_t Remote_GetLY(void)
{
	return *((volatile int16_t*)&Remote.LY);
}

int16_t Remote_GetRX(void)
{
	return *((volatile int16_t*)&Remote.RX);
}

int16_t Remote_GetRY(void)
{
	return *((volatile int16_t*)&Remote.RY);
}

int16_t Remote_GetT1(void)
{
	return *((volatile int16_t*)&Remote.T1);
}

int16_t Remote_GetS1(void)
{
	return *((volatile int16_t*)&Remote.S1);
}

uint16_t Remote_GetVA(void)
{
	return *((volatile uint16_t*)&Remote.VA);
}

int16_t Remote_GetS2(void)
{
	return *((volatile int16_t*)&Remote.S2);
}

int16_t Remote_GetT2(void)
{
	return *((volatile int16_t*)&Remote.T2);
}

uint8_t Remote_CompareChecksum(uint8_t* DataAddr, uint8_t DataLength, uint8_t Checksum)
{
	uint16_t RealChecksum = 0x0000, i;
	for(i = 0; i < DataLength; i++)
		RealChecksum += DataAddr[i];
	if(RealChecksum == Checksum)
		return 0x01;
	else
		return 0x00;
}

const uint16_t MidPulse = 1024;
const uint16_t MidError = 4;

int16_t Remote_PulseToVal(uint16_t PulseWidth)
{
	if(PulseWidth > MidPulse + MidError) 
		return PulseWidth - MidPulse - MidError;
	if(PulseWidth < MidPulse - MidError)
		return -(MidPulse - MidError - PulseWidth);
	return 0;
}

RemoteSwitch_TypeDef Remote_PulseToSwitch(uint16_t PulseWidth)
{
	if(PulseWidth == 0x0161)
		return Remote_SW0;
	if(PulseWidth == 0x0400)
		return Remote_SW1;
	if(PulseWidth == 0x069E)
		return Remote_SW2;
	return Remote_SWErr;
}

