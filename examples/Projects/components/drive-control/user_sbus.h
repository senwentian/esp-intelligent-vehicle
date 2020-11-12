#ifndef  _USER_SBUS_H
#define _USER_SBUS_H

#include "esp_types.h"

extern uint8_t SBUS_MsgPack[];
extern uint16_t SBUS_ChanelVal[];

void SBUS_Encode(void);
void SBUS_Decode(void);


#endif
