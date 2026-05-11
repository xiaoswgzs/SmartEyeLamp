#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.H"
#include "Serial.h"
#include "LED.h"
#include "string.h"
#include "Key.h"
#include "LD2410B.h"
#include "My_VL53L0X.h"
#include <stdlib.h>

uint8_t KeyNum;
uint8_t  Human_existence;
int main(void)
{
	OLED_Init();
	Key_Init();
	Serial_Init();
	LED_Init();
	LD2410B_Init();
	MyVL53L0X_Init();

//	OLED_ShowString(1,1,"TX:");
//	OLED_ShowString(3,1,"RX:");
	
	// 在 while(1) 之前定义一个变量记住雷达的上一次状态
    uint8_t Last_Human_existence = 0; 
	OLED_ShowString(1,1,"LED:");
	OLED_ShowString(2,1,"BRT:");
	uint16_t dist = 0;
	
	uint16_t Last_dist = 0;

	while(1)
	{

        
		dist = MyVL53L0X_GetDistance();
        if (abs(dist - Last_dist) > 5) 
		{
			OLED_ShowNum(3, 1, dist, 4);  
			if (dist < 500 && Last_dist >= 500)
			{
				OLED_ShowString(4, 1, "Too close");
			}
			else if (dist >= 500 && Last_dist < 500)
			{
				OLED_ShowString(4, 1, "         ");
			}
			Last_dist = dist; // 记住这次的距离
		}
		// 1. 读取雷达当前状态
		Human_existence = LD2410B_GetState();
		
		// 2. 核心逻辑：检测“人刚坐下”和“人刚离开”的瞬间
		if (Human_existence == 1 && Last_Human_existence == 0)
		{
			// 瞬间触发：人刚来！自动开灯。
			LED1_ON();
		}
		else if (Human_existence == 0 && Last_Human_existence == 1)
		{
			// 瞬间触发：人刚走！（且等完了雷达的延迟时间），自动关灯。
			LED1_OFF();
		}
		
		// 3. 更新雷达的历史状态
		Last_Human_existence = Human_existence;
		
		// 4. 处理触摸按键逻辑（无论雷达状态怎样，只要人在，或者你想随时关，都可以手动干预）
		KeyNum = Key_GetNum();
		if (KeyNum == 1)
		{
			LED1_Turn(); // 只要按了，就翻转当前灯的状态
		}
		
		// 🌟 核心调光逻辑：电平触发（只要手不松开，就一直执行）
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 1) // 持续按住 PB11（变亮）
		{
			LED1_Inc();
			Delay_ms(30); // 【魔法延时】：每次加1亮度等15毫秒。100次就是1.5秒拉满，极其平滑护眼！
		}
		
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 1)  // 持续按住 PB0（变暗）
		{
			LED1_Dec();
			Delay_ms(30); 
		}
		
		// 5. 蓝牙串口通信逻辑（保持你原来的不变）
		if (Serial_RXFlag == 1)
		{
//			OLED_ShowString(4,1,"                ");
//			OLED_ShowString(4,1,Serial_RXPacket);
			
			if (strcmp(Serial_RXPacket, "LED_ON") == 0)
			{
				LED1_ON();
				Serial_SendString("LED1_ON_OK\r\n");
//				OLED_ShowString(2,1,"                ");
//				OLED_ShowString(2,1,"LED1_ON_OK");
			}
			else if (strcmp(Serial_RXPacket, "LED_OFF") == 0)
			{
				LED1_OFF();
				Serial_SendString("LED1_OFF_OK\r\n");
//				OLED_ShowString(2,1,"                ");
//				OLED_ShowString(2,1,"LED1_OFF_OK");
			}
			else
			{
				Serial_SendString("Command_Error\r\n");
//				OLED_ShowString(2,1,"                ");
//				OLED_ShowString(2,1,"Command_Error");
			}
			Serial_RXFlag = 0;
		}
	}
}
