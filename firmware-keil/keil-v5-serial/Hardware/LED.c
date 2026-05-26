#include "stm32f10x.h"
#include "PWM.h"
#include "LED.h"

#define LED_LEVEL_MAX       100
#define LED_MEMORY_MIN       10
#define LED_TASK_MS           8
#define LED_FADE_STEP_Q8     96

static uint8_t Brightness_Set = 70;
static uint8_t Brightness_Target = 0;
static uint16_t Brightness_CurrentQ8 = 0;
static uint32_t LedLastTime = 0;

static uint16_t LED_LevelToPwm(uint8_t level)
{
    uint32_t pwm;
    uint32_t top;

    if (level == 0)
    {
        return 0;
    }

    top = PWM_GetTopValue();

    /*
     * 人眼对亮度不是线性感知。
     * 这里用平方曲线把 0-100 的“感觉亮度”映射到 PWM 占空比。
     * 低亮度更细，高亮度不刺眼，调光会更像成品灯具。
     */
    pwm = 2 + ((uint32_t)level * level * (top - 2) + 5000) / 10000;

    if (pwm > top)
    {
        pwm = top;
    }

    return (uint16_t)pwm;
}

static uint8_t LED_Q8ToLevel(uint16_t levelQ8)
{
    return (uint8_t)((levelQ8 + 128) >> 8);
}

static void LED_ApplyCurrent(void)
{
    uint8_t level;

    level = LED_Q8ToLevel(Brightness_CurrentQ8);
    PWM_SetCompare3(LED_LevelToPwm(level));
}

static void LED_SaveMemory(uint8_t level)
{
    if (level > LED_MEMORY_MIN)
    {
        Brightness_Set = level;
    }
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
    if (Brightness_Target > 0 || Brightness_CurrentQ8 > 0)
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
    LED1_AddTarget(2);
}

void LED1_Dec(void)
{
    LED1_AddTarget(-2);
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

void LED1_Task(uint32_t nowMs)
{
    uint16_t targetQ8;

    if ((uint32_t)(nowMs - LedLastTime) < LED_TASK_MS)
    {
        return;
    }

    LedLastTime = nowMs;
    targetQ8 = ((uint16_t)Brightness_Target) << 8;

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
    LED1_SetNow(0);
}
