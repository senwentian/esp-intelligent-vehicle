#ifndef _USER_MATH_H
#define _USER_MATH_H

#include "esp_types.h"



void absLimit(float* Val, float Limit);

float Min_f(float x, float y);
float Max_f(float x, float y);

// float MinAbs_f(float x, float y);
// float MaxAbs_f(float x, float y);

//Reciprocal square root平方根倒数
float InvSqrt(float Val);

uint8_t ValueInRange_u(uint32_t Value, uint32_t Min, uint32_t Max);
uint8_t ValueInRange_i(int32_t Value, int32_t Min, int32_t Max);
uint8_t ValueInRange_f(float Value, float Min, float Max);
float FlexibelValue(float dstVal, float srcVal, float step);

#endif
