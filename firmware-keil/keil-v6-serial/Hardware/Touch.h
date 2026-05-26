#ifndef __TOUCH_H
#define __TOUCH_H

#include "stm32f10x.h"

typedef enum
{
    TOUCH_EVENT_NONE = 0,
    TOUCH_EVENT_MODE,
    TOUCH_EVENT_BRIGHT_UP,
    TOUCH_EVENT_BRIGHT_DOWN
} TouchEvent_t;

void Touch_Init(void);
void Touch_Task(uint32_t nowMs);
TouchEvent_t Touch_TakeEvent(void);

#endif
