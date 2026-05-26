#include "stm32f10x.h"
#include "PWM.h"

#define PWM_TOP_VALUE 1000

void PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_InternalClockConfig(TIM2);

    /*
     * 72MHz / 4 / 1000 = 18kHz
     * 比 100 级 PWM 更细，频率也仍然高于可见频闪范围。
     */
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = PWM_TOP_VALUE - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 4 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);

    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

void PWM_SetCompare3(uint16_t Compare)
{
    if (Compare > PWM_TOP_VALUE)
    {
        Compare = PWM_TOP_VALUE;
    }

    TIM_SetCompare3(TIM2, Compare);
}

uint16_t PWM_GetTopValue(void)
{
    return PWM_TOP_VALUE;
}
