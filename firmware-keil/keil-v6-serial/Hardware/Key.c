#include "stm32f10x.h"

void Key_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Mode = GPIO_Mode_IPD;
    gpio.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11 | GPIO_Pin_0;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
}

uint8_t Key_GetNum(void)
{
    static uint8_t last1 = 0;
    static uint8_t last2 = 0;
    static uint8_t last3 = 0;
    uint8_t now1;
    uint8_t now2;
    uint8_t now3;
    uint8_t key;

    key = 0;
    now1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
    now2 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
    now3 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);

    if (now1 && !last1)
    {
        key = 1;
    }
    else if (now2 && !last2)
    {
        key = 2;
    }
    else if (now3 && !last3)
    {
        key = 3;
    }

    last1 = now1;
    last2 = now2;
    last3 = now3;

    return key;
}
