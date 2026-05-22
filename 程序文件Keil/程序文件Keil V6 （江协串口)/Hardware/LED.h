#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

void LED_Init(void);
void LED1_Task(uint32_t nowMs);

void LED1_ON(void);
void LED1_OFF(void);
void LED1_Turn(void);
void LED1_Inc(void);
void LED1_Dec(void);

void LED1_SetNow(uint8_t level);
void LED1_SetTarget(uint8_t level);
void LED1_AddTarget(int8_t step);
void LED1_AckBlink(uint8_t count);

uint8_t LED1_IsOn(void);
uint8_t LED1_GetBrightness(void);
uint8_t LED1_GetTarget(void);
uint8_t Get_Brightness_Set(void);

#endif
