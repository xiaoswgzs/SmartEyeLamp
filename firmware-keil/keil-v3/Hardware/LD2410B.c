#include "stm32f10x.h"                  // Device header
#include "Delay.h"

void LD2410B_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; // 【修改这里】改为下拉输入 (Input Pull Down)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
}


uint8_t LD2410B_GetState(void)
{
    uint8_t LD2410B_State = 0;
	LD2410B_State = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
    
    return LD2410B_State;
}
