#include "Gesture_ToF.h"

#define GESTURE_VALID_MIN_MM      40
#define GESTURE_VALID_MAX_MM    1200
#define GESTURE_ENTER_MM         190
#define GESTURE_LEAVE_MM         305

#define GESTURE_WAVE_MIN_MS       70
#define GESTURE_WAVE_MAX_MS      480
#define GESTURE_HOLD_MS          620

#define FILTER_INIT_MM          8190

typedef enum
{
    G_STATE_IDLE = 0,
    G_STATE_NEAR,
    G_STATE_HOLDING
} GestureState_t;

static GestureState_t s_state = G_STATE_IDLE;
static uint16_t s_filteredDistance = FILTER_INIT_MM;
static uint16_t s_lastDistance = FILTER_INIT_MM;
static uint32_t s_nearStartTime = 0;

static uint8_t Gesture_IsValid(uint16_t distanceMm)
{
    if (distanceMm >= GESTURE_VALID_MIN_MM && distanceMm <= GESTURE_VALID_MAX_MM)
    {
        return 1;
    }
    return 0;
}

static uint16_t Gesture_Filter(uint16_t distanceMm)
{
    if (s_filteredDistance == FILTER_INIT_MM)
    {
        s_filteredDistance = distanceMm;
    }
    else
    {
        /* 响应优先：1/2 新数据 + 1/2 旧数据，减少“慢半拍”的感觉。 */
        s_filteredDistance = (uint16_t)((s_filteredDistance + distanceMm) / 2);
    }

    s_lastDistance = s_filteredDistance;
    return s_filteredDistance;
}

void Gesture_Init(void)
{
    s_state = G_STATE_IDLE;
    s_filteredDistance = FILTER_INIT_MM;
    s_lastDistance = FILTER_INIT_MM;
    s_nearStartTime = 0;
}

uint16_t Gesture_GetDistance(void)
{
    return s_lastDistance;
}

uint8_t Gesture_IsHandNear(void)
{
    if (s_state == G_STATE_NEAR || s_state == G_STATE_HOLDING)
    {
        return 1;
    }
    return 0;
}

GestureEvent_t Gesture_Update(uint16_t distanceMm, uint32_t nowMs)
{
    uint16_t d;
    uint32_t nearTime;

    if (Gesture_IsValid(distanceMm))
    {
        d = Gesture_Filter(distanceMm);
    }
    else
    {
        d = GESTURE_VALID_MAX_MM + 1;
    }

    switch (s_state)
    {
        case G_STATE_IDLE:
            if (d < GESTURE_ENTER_MM)
            {
                s_state = G_STATE_NEAR;
                s_nearStartTime = nowMs;
                return GESTURE_EVENT_NEAR;
            }
            break;

        case G_STATE_NEAR:
            nearTime = nowMs - s_nearStartTime;

            if (d > GESTURE_LEAVE_MM)
            {
                s_state = G_STATE_IDLE;
                if (nearTime >= GESTURE_WAVE_MIN_MS && nearTime <= GESTURE_WAVE_MAX_MS)
                {
                    return GESTURE_EVENT_WAVE;
                }
                return GESTURE_EVENT_NONE;
            }

            if (nearTime >= GESTURE_HOLD_MS)
            {
                s_state = G_STATE_HOLDING;
                return GESTURE_EVENT_HOLD_START;
            }
            break;

        case G_STATE_HOLDING:
            if (d > GESTURE_LEAVE_MM)
            {
                s_state = G_STATE_IDLE;
                return GESTURE_EVENT_HOLD_END;
            }
            return GESTURE_EVENT_HOLDING;

        default:
            s_state = G_STATE_IDLE;
            break;
    }

    return GESTURE_EVENT_NONE;
}
