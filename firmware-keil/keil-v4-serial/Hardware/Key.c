#include "stm32f10x.h"                  // Device header
#include "Delay.h"

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; // 【修改这里】改为下拉输入 (Input Pull Down)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11 | GPIO_Pin_0 ;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
}


uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;
    static uint8_t LastState1 = 0; // 静态变量，记住按键1上一次的状态
    uint8_t CurrentState1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
    
    // 如果现在摸了(1)，且上一次没摸(0)，说明是【手指刚贴上去的一瞬间】
    if (CurrentState1 == 1 && LastState1 == 0)
    {
        Delay_ms(20); // 延时20ms消抖
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 1) // 再次确认真的摸了
        {
            KeyNum = 1;
        }
    }
    LastState1 = CurrentState1; // 更新状态
    
//    // ----- 如果你想用按键2 (PB11)，同理 -----
//    static uint8_t LastState2 = 0; 
//    uint8_t CurrentState2 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
//    if (CurrentState2 == 1 && LastState2 == 0)
//    {
//        Delay_ms(20);
//        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 1)
//        {
//            KeyNum = 2;
//        }
//    }
//    LastState2 = CurrentState2;
//    // ----------------------------------------
//	
//	// ----- 如果你想用按键3 (PB0)，同理 -----
//    static uint8_t LastState3 = 0; 
//    uint8_t CurrentState3 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);
//    if (CurrentState3 == 1 && LastState3 == 0)
//    {
//        Delay_ms(20);
//        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 1)
//        {
//            KeyNum = 3;
//        }
//    }
//    LastState3 = CurrentState3;
//    // ----------------------------------------
    
    return KeyNum;
}
