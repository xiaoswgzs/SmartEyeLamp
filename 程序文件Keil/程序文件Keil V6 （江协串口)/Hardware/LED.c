#include "stm32f10x.h"
#include "PWM.h"
#include "LED.h"

#define LED_LEVEL_MAX          100
#define LED_MEMORY_MIN          10

#define LED_TASK_MS              6
#define LED_FADE_STEP_Q8       512

#define LED_ACK_PERIOD_MS      120
#define LED_ACK_HIGH            58
#define LED_ACK_LOW             12

typedef struct
{
    uint8_t Active;
    uint8_t Count;
    uint32_t StartTime;
} LED_Ack_t;

static uint8_t Brightness_Set = 70;
static uint8_t Brightness_Target = 0;
static uint16_t Brightness_CurrentQ8 = 0;
static uint32_t LedLastTime = 0;
static LED_Ack_t Ack;

static uint8_t LED_Q8ToLevel(uint16_t levelQ8)
{
    return (uint8_t)((levelQ8 + 128) >> 8);
}

static uint16_t LED_LevelToPwm(uint8_t level)
{
    uint32_t top;
    uint32_t usable;
    uint32_t linear;
    uint32_t curve;
    uint32_t pwm;

    if (level == 0)
    {
        return 0;
    }

    top = PWM_GetTopValue();
    usable = top - 3;

    /*
     * Soft-linear curve.
     * Pure square curve is gentle but feels weak at low brightness.
     * This keeps low levels responsive while still avoiding a harsh jump.
     */
    linear = ((uint32_t)level * usable + 50) / 100;
    curve = ((uint32_t)level * level * usable + 5000) / 10000;
    pwm = 3 + (linear + curve) / 2;

    if (pwm > top)
    {
        pwm = top;
    }

    return (uint16_t)pwm;
}

static void LED_ApplyCurrent(void)
{
    PWM_SetCompare3(LED_LevelToPwm(LED_Q8ToLevel(Brightness_CurrentQ8)));
}

static void LED_SaveMemory(uint8_t level)
{
    if (level > LED_MEMORY_MIN)
    {
        Brightness_Set = level;
    }
}

static uint8_t LED_GetEffectTarget(uint32_t nowMs)
{
    uint32_t elapsed;
    uint32_t phase;
    uint32_t total;

    if (Ack.Active == 0)
    {
        return Brightness_Target;
    }

    elapsed = nowMs - Ack.StartTime;
    total = (uint32_t)Ack.Count * LED_ACK_PERIOD_MS;

    if (elapsed >= total)
    {
        Ack.Active = 0;
        return Brightness_Target;
    }

    phase = elapsed % LED_ACK_PERIOD_MS;

    if (phase < (LED_ACK_PERIOD_MS / 2))
    {
        if (Brightness_Target > LED_ACK_HIGH)
        {
            return Brightness_Target;
        }
        return LED_ACK_HIGH;
    }

    if (Brightness_Target == 0)
    {
        return LED_ACK_LOW;
    }

    if (Brightness_Target > 30)
    {
        return (uint8_t)(Brightness_Target - 18);
    }

    return LED_ACK_LOW;
}

uint8_t Get_Brightness_Set(void)
{
    return Brightness_Set;
}

uint8_t LED1_GetBrightness(void)
{
    return LED_Q8ToLevel(Brightness_CurrentQ8);
}

uint8_t LED1_GetTarget(void)
{
    return Brightness_Target;
}

uint8_t LED1_IsOn(void)
{
    if (Brightness_Target > 0 || Brightness_CurrentQ8 > 0 || Ack.Active)
    {
        return 1;
    }

    return 0;
}

void LED1_SetTarget(uint8_t level)
{
    if (level > LED_LEVEL_MAX)
    {
        level = LED_LEVEL_MAX;
    }

    Brightness_Target = level;
    LED_SaveMemory(level);
}

void LED1_AddTarget(int8_t step)
{
    int16_t level;

    level = Brightness_Target;
    level += step;

    if (level < 0)
    {
        level = 0;
    }
    else if (level > LED_LEVEL_MAX)
    {
        level = LED_LEVEL_MAX;
    }

    LED1_SetTarget((uint8_t)level);
}

void LED1_Inc(void)
{
    LED1_AddTarget(4);
}

void LED1_Dec(void)
{
    LED1_AddTarget(-4);
}

void LED1_ON(void)
{
    if (Brightness_Set < LED_MEMORY_MIN)
    {
        Brightness_Set = LED_MEMORY_MIN;
    }

    Brightness_Target = Brightness_Set;
}

void LED1_OFF(void)
{
    Brightness_Target = 0;
}

void LED1_Turn(void)
{
    if (Brightness_Target == 0)
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

    Brightness_Target = level;
    Brightness_CurrentQ8 = ((uint16_t)level) << 8;
    LED_SaveMemory(level);
    LED_ApplyCurrent();
}

void LED1_AckBlink(uint8_t count)
{
    if (count == 0)
    {
        count = 1;
    }
    if (count > 5)
    {
        count = 5;
    }

    Ack.Active = 1;
    Ack.Count = count;
    Ack.StartTime = 0;
}

void LED1_Task(uint32_t nowMs)
{
    uint16_t targetQ8;
    uint8_t displayTarget;

    if ((uint32_t)(nowMs - LedLastTime) < LED_TASK_MS)
    {
        return;
    }

    LedLastTime = nowMs;

    if (Ack.Active && Ack.StartTime == 0)
    {
        Ack.StartTime = nowMs;
    }

    displayTarget = LED_GetEffectTarget(nowMs);
    targetQ8 = ((uint16_t)displayTarget) << 8;

    if (Brightness_CurrentQ8 < targetQ8)
    {
        if ((uint16_t)(targetQ8 - Brightness_CurrentQ8) <= LED_FADE_STEP_Q8)
        {
            Brightness_CurrentQ8 = targetQ8;
        }
        else
        {
            Brightness_CurrentQ8 += LED_FADE_STEP_Q8;
        }

        LED_ApplyCurrent();
    }
    else if (Brightness_CurrentQ8 > targetQ8)
    {
        if ((uint16_t)(Brightness_CurrentQ8 - targetQ8) <= LED_FADE_STEP_Q8)
        {
            Brightness_CurrentQ8 = targetQ8;
        }
        else
        {
            Brightness_CurrentQ8 -= LED_FADE_STEP_Q8;
        }

        LED_ApplyCurrent();
    }
}

void LED_Init(void)
{
    PWM_Init();
    Ack.Active = 0;
    Ack.Count = 0;
    Ack.StartTime = 0;
    LED1_SetNow(0);
}
