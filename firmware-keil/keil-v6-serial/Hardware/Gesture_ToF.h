#ifndef __GESTURE_TOF_H
#define __GESTURE_TOF_H

#include "stm32f10x.h"

typedef enum
{
    GESTURE_EVENT_NONE = 0,
    GESTURE_EVENT_NEAR,
    GESTURE_EVENT_WAVE,
    GESTURE_EVENT_HOLD_START,
    GESTURE_EVENT_HOLDING,
    GESTURE_EVENT_HOLD_END
} GestureEvent_t;

void Gesture_Init(void);
GestureEvent_t Gesture_Update(uint16_t distanceMm, uint32_t nowMs);
uint16_t Gesture_GetDistance(void);
uint8_t Gesture_IsHandNear(void);

#endif
