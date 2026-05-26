#include "SystemTime.h"

static volatile uint32_t s_ms = 0;

void SystemTime_Init(void)
{
    TIM_TimeBaseInitTypeDef tim;
    NVIC_InitTypeDef nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    tim.TIM_Prescaler = 7200 - 1;      /* 72MHz / 7200 = 10kHz */
    tim.TIM_Period = 10 - 1;           /* 10kHz / 10 = 1kHz */
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &tim);

    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    nvic.NVIC_IRQChannel = TIM4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;
    nvic.NVIC_IRQChannelSubPriority = 2;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM4, ENABLE);
}

uint32_t SystemTime_Millis(void)
{
    return s_ms;
}

uint8_t SystemTime_IsDue(uint32_t *lastTime, uint16_t intervalMs)
{
    uint32_t now;

    now = SystemTime_Millis();
    if ((uint32_t)(now - *lastTime) >= intervalMs)
    {
        *lastTime = now;
        return 1;
    }

    return 0;
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
    {
        s_ms++;
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}
