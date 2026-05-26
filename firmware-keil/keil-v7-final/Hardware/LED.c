#include "LED.h"
#include "PWM.h"

#define LED_LEVEL_MAX             100
#define LED_MEMORY_MIN             10

#define LED_TASK_MS                 4
#define LED_FADE_STEP_Q8          640

#define LED_ACK_HIGH_MS            70
#define LED_ACK_LOW_MS             50
#define LED_ACK_LEVEL_MIN          34
#define LED_ACK_LIFT               24

typedef struct
{
    uint8_t memoryLevel;
    uint8_t baseTarget;
    uint16_t currentQ8;
    uint32_t lastTaskTime;

    uint8_t ackActive;
    uint8_t ackHigh;
    uint8_t ackRemainEdges;
    uint32_t ackNextTime;
} LedState_t;

static LedState_t s_led;

static uint8_t LED_ClampLevel(int16_t level)
{
    if (level < 0)
    {
        return 0;
    }
    if (level > LED_LEVEL_MAX)
    {
        return LED_LEVEL_MAX;
    }
    return (uint8_t)level;
}

static uint8_t LED_Q8ToLevel(uint16_t levelQ8)
{
    return (uint8_t)((levelQ8 + 128) >> 8);
}

static uint16_t LED_LevelToPwm(uint8_t level)
{
    uint32_t top;
    uint32_t pwm;

    if (level == 0)
    {
        return 0;
    }

    top = PWM_GetTopValue();

    /* level 是体感亮度，PWM 用轻微平方曲线，低亮区更柔。 */
    pwm = 1 + ((uint32_t)level * level * (top - 1) + 5000) / 10000;
    if (pwm > top)
    {
        pwm = top;
    }
    return (uint16_t)pwm;
}

static void LED_SaveMemory(uint8_t level)
{
    if (level > LED_MEMORY_MIN)
    {
        s_led.memoryLevel = level;
    }
}

static void LED_ApplyCurrent(void)
{
    PWM_SetCompare3(LED_LevelToPwm(LED_Q8ToLevel(s_led.currentQ8)));
}

static uint8_t LED_GetAckTarget(uint32_t nowMs)
{
    uint8_t highLevel;
    uint8_t lowLevel;

    if (!s_led.ackActive)
    {
        return s_led.baseTarget;
    }

    if ((int32_t)(nowMs - s_led.ackNextTime) >= 0)
    {
        s_led.ackHigh = !s_led.ackHigh;

        if (s_led.ackRemainEdges > 0)
        {
            s_led.ackRemainEdges--;
        }

        if (s_led.ackRemainEdges == 0)
        {
            s_led.ackActive = 0;
            return s_led.baseTarget;
        }

        s_led.ackNextTime = nowMs + (s_led.ackHigh ? LED_ACK_HIGH_MS : LED_ACK_LOW_MS);
    }

    if (s_led.ackHigh)
    {
        highLevel = s_led.baseTarget;
        if (highLevel < LED_ACK_LEVEL_MIN)
        {
            highLevel = LED_ACK_LEVEL_MIN;
        }
        else if (highLevel + LED_ACK_LIFT < LED_LEVEL_MAX)
        {
            highLevel = (uint8_t)(highLevel + LED_ACK_LIFT);
        }
        else
        {
            highLevel = LED_LEVEL_MAX;
        }
        return highLevel;
    }

    if (s_led.baseTarget > 22)
    {
        lowLevel = (uint8_t)(s_led.baseTarget - 22);
    }
    else
    {
        lowLevel = 0;
    }
    return lowLevel;
}

uint8_t Get_Brightness_Set(void)
{
    return s_led.memoryLevel;
}

uint8_t LED1_GetBrightness(void)
{
    return LED_Q8ToLevel(s_led.currentQ8);
}

uint8_t LED1_GetTarget(void)
{
    return s_led.baseTarget;
}

uint8_t LED1_IsOn(void)
{
    return s_led.baseTarget > 0 ? 1 : 0;
}

void LED1_SetMemory(uint8_t level)
{
    if (level > LED_LEVEL_MAX)
    {
        level = LED_LEVEL_MAX;
    }
    if (level < LED_MEMORY_MIN)
    {
        level = LED_MEMORY_MIN;
    }
    s_led.memoryLevel = level;
}

void LED1_AddMemory(int8_t step)
{
    int16_t level;

    level = (int16_t)s_led.memoryLevel + step;
    LED1_SetMemory(LED_ClampLevel(level));
}

void LED1_SetTarget(uint8_t level)
{
    if (level > LED_LEVEL_MAX)
    {
        level = LED_LEVEL_MAX;
    }

    s_led.baseTarget = level;
    LED_SaveMemory(level);
}

void LED1_AddTarget(int8_t step)
{
    int16_t level;

    level = (int16_t)s_led.baseTarget + step;
    LED1_SetTarget(LED_ClampLevel(level));
}

void LED1_Inc(void)
{
    LED1_AddTarget(5);
}

void LED1_Dec(void)
{
    LED1_AddTarget(-5);
}

void LED1_ON(void)
{
    if (s_led.memoryLevel < LED_MEMORY_MIN)
    {
        s_led.memoryLevel = LED_MEMORY_MIN;
    }
    s_led.baseTarget = s_led.memoryLevel;
}

void LED1_OFF(void)
{
    s_led.baseTarget = 0;
}

void LED1_Turn(void)
{
    if (s_led.baseTarget == 0)
    {
        LED1_ON();
    }
    else
    {
        LED1_OFF();
    }
}

void LED1_SetNow(uint8_t level)
{
    if (level > LED_LEVEL_MAX)
    {
        level = LED_LEVEL_MAX;
    }

    s_led.baseTarget = level;
    s_led.currentQ8 = ((uint16_t)level) << 8;
    LED_SaveMemory(level);
    LED_ApplyCurrent();
}

void LED1_AckFlash(uint8_t count)
{
    if (count == 0)
    {
        return;
    }
    if (count > 4)
    {
        count = 4;
    }

    s_led.ackActive = 1;
    s_led.ackHigh = 1;
    s_led.ackRemainEdges = (uint8_t)(count * 2);
    s_led.ackNextTime = 0;
}

void LED1_Task(uint32_t nowMs)
{
    uint8_t outputTarget;
    uint16_t targetQ8;

    if ((uint32_t)(nowMs - s_led.lastTaskTime) < LED_TASK_MS)
    {
        return;
    }

    s_led.lastTaskTime = nowMs;
    outputTarget = LED_GetAckTarget(nowMs);
    targetQ8 = ((uint16_t)outputTarget) << 8;

    if (s_led.currentQ8 < targetQ8)
    {
        if ((uint16_t)(targetQ8 - s_led.currentQ8) <= LED_FADE_STEP_Q8)
        {
            s_led.currentQ8 = targetQ8;
        }
        else
        {
            s_led.currentQ8 += LED_FADE_STEP_Q8;
        }
        LED_ApplyCurrent();
    }
    else if (s_led.currentQ8 > targetQ8)
    {
        if ((uint16_t)(s_led.currentQ8 - targetQ8) <= LED_FADE_STEP_Q8)
        {
            s_led.currentQ8 = targetQ8;
        }
        else
        {
            s_led.currentQ8 -= LED_FADE_STEP_Q8;
        }
        LED_ApplyCurrent();
    }
}

void LED_Init(void)
{
    PWM_Init();

    s_led.memoryLevel = 65;
    s_led.baseTarget = 0;
    s_led.currentQ8 = 0;
    s_led.lastTaskTime = 0;
    s_led.ackActive = 0;
    s_led.ackHigh = 0;
    s_led.ackRemainEdges = 0;
    s_led.ackNextTime = 0;

    LED_ApplyCurrent();
}
