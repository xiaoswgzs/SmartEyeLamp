#ifndef __APP_H
#define __APP_H

#include "stm32f10x.h"

typedef enum
{
    APP_MODE_MANUAL = 0,
    APP_MODE_AUTO = 1
} AppMode_t;

void App_Init(void);
void App_Task(void);

#endif
