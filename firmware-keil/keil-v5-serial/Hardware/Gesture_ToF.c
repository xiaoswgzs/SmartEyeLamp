#include "Gesture_ToF.h"

#define GESTURE_VALID_MIN_MM      40
#define GESTURE_VALID_MAX_MM    1200
#define GESTURE_ENTER_MM         180
#define GESTURE_LEAVE_MM         280

#define GESTURE_WAVE_MIN_MS       80
#define GESTURE_WAVE_MAX_MS      520
#define GESTURE_HOLD_MS          750
#define GESTURE_DOUBLE_MS        650

#define FILTER_INIT_MM          8190

typedef enum
{
    G_STATE_IDLE = 0,
    G_STATE_NEAR,
    G_STATE_HOLDING
} GestureState_t;

static GestureState_t State = G_STATE_IDLE;
static uint16_t FilteredDistance = FILTER_INIT_MM;
static uint16_t LastDistance = FILTER_INIT_MM;
static uint32_t NearStartTime = 0;
static uint32_t LastWaveTime = 0;
static uint8_t HoldReported = 0;

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
    if (FilteredDistance == FILTER_INIT_MM)
    {
        FilteredDistance = distanceMm;
    }
    else
    {
        /* 1/4 新数据 + 3/4 旧数据，压掉 VL53L0X 的小抖动。 */
        FilteredDistance = (uint16_t)((FilteredDistance * 3 + distanceMm) / 4);
    }

    LastDistance = FilteredDistance;
    return FilteredDistance;
}

void Gesture_Init(void)
{
    State = G_STATE_IDLE;
    FilteredDistance = FILTER_INIT_MM;
    LastDistance = FILTER_INIT_MM;
    NearStartTime = 0;
    LastWaveTime = 0;
    HoldReported = 0;
}

uint16_t Gesture_GetDistance(void)
{
    return LastDistance;
}

uint8_t Gesture_IsHandNear(void)
{
    if (State == G_STATE_NEAR || State == G_STATE_HOLDING)
    {
        return 1;
    }

    return 0;
}

GestureEvent_t Gesture_Update(uint16_t distanceMm, uint32_t nowMs)
{
    uint16_t d;
    uint32_t nearTime;

    if (!Gesture_IsValid(distanceMm))
    {
        d = GESTURE_VALID_MAX_MM + 1;
    }
    else
    {
        d = Gesture_Filter(distanceMm);
    }

    switch (State)
    {
        case G_STATE_IDLE:
            if (d < GESTURE_ENTER_MM)
            {
                State = G_STATE_NEAR;
                NearStartTime = nowMs;
                HoldReported = 0;
                return GESTURE_EVENT_NEAR;
            }
            break;

        case G_STATE_NEAR:
            nearTime = nowMs - NearStartTime;

            if (d > GESTURE_LEAVE_MM)
            {
                State = G_STATE_IDLE;

                if (nearTime >= GESTURE_WAVE_MIN_MS && nearTime <= GESTURE_WAVE_MAX_MS)
                {
                    if ((uint32_t)(nowMs - LastWaveTime) <= GESTURE_DOUBLE_MS)
                    {
                        LastWaveTime = 0;
                        return GESTURE_EVENT_DOUBLE_WAVE;
                    }

                    LastWaveTime = nowMs;
                    return GESTURE_EVENT_WAVE;
                }

                return GESTURE_EVENT_FAR;
            }

            if (nearTime >= GESTURE_HOLD_MS)
            {
                State = G_STATE_HOLDING;
                HoldReported = 1;
                return GESTURE_EVENT_HOLD_START;
            }
            break;

        case G_STATE_HOLDING:
            if (d > GESTURE_LEAVE_MM)
            {
                State = G_STATE_IDLE;
                HoldReported = 0;
                return GESTURE_EVENT_HOLD_END;
            }

            if (HoldReported)
            {
                return GESTURE_EVENT_HOLDING;
            }
            break;

        default:
            State = G_STATE_IDLE;
            break;
    }

    return GESTURE_EVENT_NONE;
}
