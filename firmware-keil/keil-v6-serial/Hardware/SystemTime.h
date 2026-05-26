#ifndef __SYSTEM_TIME_H
#define __SYSTEM_TIME_H

#include "stm32f10x.h"

void SystemTime_Init(void);
uint32_t SystemTime_Millis(void);
uint8_t SystemTime_IsDue(uint32_t *lastTime, uint16_t intervalMs);

#endif
