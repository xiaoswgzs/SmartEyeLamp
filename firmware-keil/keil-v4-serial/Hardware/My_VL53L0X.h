#ifndef __MY_VL53L0X_H
#define __MY_VL53L0X_H

#include "stdint.h"   // 解决 uint16_t 未定义
#include "vl53l0x_api.h" // 解决 VL53L0X_DEV 未定义

void MyVL53L0X_Init(void);
uint16_t MyVL53L0X_GetDistance(void);

#endif
