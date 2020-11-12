#ifndef _USER_REMOTE_H
#define _USER_REMOTE_H

#include "esp_types.h"
#include "user_sbus.h"
#include "user_uart.h"
#include "user_motor.h"
#include "user_math.h"

#define SBUS_IN 1
#define UPWM_IN 2

#define RCV_SIG SBUS_IN

extern volatile uint8_t RemoteUpdated;

typedef struct Remote_t
{
	int16_t LX;
	int16_t LY;
	int16_t RX;
	int16_t RY;
	int16_t T1;
	int16_t S1;
	uint16_t VA;
	int16_t S2;
	int16_t T2;
} Remote_t;

typedef enum
{
	Remote_SW0 = 0x00,
	Remote_SW1 = 0x01,
	Remote_SW2 = 0x02,
	Remote_SWErr = 0xFF,
} RemoteSwitch_TypeDef;

typedef enum
{
	Remote_0 = 0x00,
	Remote_1 = 0x01,
	Remote_Err = 0xFF,
} RemoteTwoSwitch_TypeDef;

void UpdateRemoteInfo(void *PWM_Array);
uint16_t Remote_GetChanalVal(uint8_t Chanel);
int16_t Remote_GetLX(void);
int16_t Remote_GetLY(void);
int16_t Remote_GetRX(void);
int16_t Remote_GetRY(void);
int16_t Remote_GetS1(void);
int16_t Remote_GetT1(void);
uint16_t Remote_GetVA(void);
int16_t Remote_GetS2(void);
int16_t Remote_GetT2(void);
uint8_t Remote_CompareChecksum(uint8_t *DataAddr, uint8_t DataLength, uint8_t Checksum);
int16_t Remote_PulseToVal(uint16_t PulseWidth);
RemoteSwitch_TypeDef Remote_PulseToSwitch(uint16_t PulseWidth);
RemoteTwoSwitch_TypeDef Remote_PulseToSwitch_Two(uint16_t PulseWidth);

#endif
