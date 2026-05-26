#include "Touch.h"

#define TOUCH_SCAN_MS        5
#define TOUCH_DEBOUNCE_MS   12
#define TOUCH_REPEAT_MS     18

#define TOUCH_PORT          GPIOB
#define TOUCH_MODE_PIN      GPIO_Pin_1
#define TOUCH_UP_PIN        GPIO_Pin_11
#define TOUCH_DOWN_PIN      GPIO_Pin_0

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t raw;
    uint8_t stable;
    uint8_t pressEvent;
    uint32_t changeTime;
} TouchKey_t;

static TouchKey_t s_modeKey;
static TouchKey_t s_upKey;
static TouchKey_t s_downKey;
static TouchEvent_t s_pendingEvent = TOUCH_EVENT_NONE;
static uint32_t s_scanTick = 0;
static uint32_t s_repeatTick = 0;

static void TouchKey_InitState(TouchKey_t *key, GPIO_TypeDef *port, uint16_t pin)
{
    key->port = port;
    key->pin = pin;
    key->raw = GPIO_ReadInputDataBit(port, pin);
    key->stable = key->raw;
    key->pressEvent = 0;
    key->changeTime = 0;
}

static void TouchKey_Update(TouchKey_t *key, uint32_t nowMs)
{
    uint8_t raw;

    raw = GPIO_ReadInputDataBit(key->port, key->pin);
    if (raw != key->raw)
    {
        key->raw = raw;
        key->changeTime = nowMs;
    }

    if ((uint32_t)(nowMs - key->changeTime) >= TOUCH_DEBOUNCE_MS)
    {
        if (key->stable != key->raw)
        {
            key->stable = key->raw;
            if (key->stable)
            {
                key->pressEvent = 1;
            }
        }
    }
}

static uint8_t TouchKey_TakePress(TouchKey_t *key)
{
    if (key->pressEvent)
    {
        key->pressEvent = 0;
        return 1;
    }
    return 0;
}

static void Touch_Emit(TouchEvent_t event)
{
    if (s_pendingEvent == TOUCH_EVENT_NONE)
    {
        s_pendingEvent = event;
    }
}

void Touch_Init(void)
{
    TouchKey_InitState(&s_modeKey, TOUCH_PORT, TOUCH_MODE_PIN);
    TouchKey_InitState(&s_upKey, TOUCH_PORT, TOUCH_UP_PIN);
    TouchKey_InitState(&s_downKey, TOUCH_PORT, TOUCH_DOWN_PIN);
}

void Touch_Task(uint32_t nowMs)
{
    if ((uint32_t)(nowMs - s_scanTick) < TOUCH_SCAN_MS)
    {
        return;
    }
    s_scanTick = nowMs;

    TouchKey_Update(&s_modeKey, nowMs);
    TouchKey_Update(&s_upKey, nowMs);
    TouchKey_Update(&s_downKey, nowMs);

    if (TouchKey_TakePress(&s_modeKey))
    {
        Touch_Emit(TOUCH_EVENT_MODE);
    }

    if (TouchKey_TakePress(&s_upKey) && !s_downKey.stable)
    {
        s_repeatTick = nowMs;
        Touch_Emit(TOUCH_EVENT_BRIGHT_UP);
    }
    else if (TouchKey_TakePress(&s_downKey) && !s_upKey.stable)
    {
        s_repeatTick = nowMs;
        Touch_Emit(TOUCH_EVENT_BRIGHT_DOWN);
    }

    if ((uint32_t)(nowMs - s_repeatTick) >= TOUCH_REPEAT_MS)
    {
        if (s_upKey.stable && !s_downKey.stable)
        {
            s_repeatTick = nowMs;
            Touch_Emit(TOUCH_EVENT_BRIGHT_UP);
        }
        else if (s_downKey.stable && !s_upKey.stable)
        {
            s_repeatTick = nowMs;
            Touch_Emit(TOUCH_EVENT_BRIGHT_DOWN);
        }
        else
        {
            s_repeatTick = nowMs;
        }
    }
}

TouchEvent_t Touch_TakeEvent(void)
{
    TouchEvent_t event;

    event = s_pendingEvent;
    s_pendingEvent = TOUCH_EVENT_NONE;
    return event;
}
