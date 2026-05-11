#include "stm32f10x.h"                  // Device header
#include "PWM.h"
#include "OLED.H"

static uint8_t Brightness_Set = 100;
static uint8_t Brightness = 0;

uint8_t Get_Brightness_Set()
{
	return Brightness_Set;
}

void LED1_Inc(void)
{
    if (Brightness < 100) // 限制最大亮度不能超过100
    {
        Brightness++;
        
        // 🌟 核心修复：把当前亮度存入记忆！
        Brightness_Set = Brightness; 
        
        PWM_SetCompare3(Brightness);
		OLED_ShowString(1,5,"ON ");
		OLED_ShowNum(2,5,Brightness,3);
    }
}

void LED1_Dec(void)
{
    if (Brightness > 0) // 限制最小亮度不能低于0
    {
        Brightness--;
        
		
		
		
        // 🌟 防坑设计：如果被调到了0（灯灭了），我们就不记0，保留它熄灭前的最低亮度。
        // 这样下次按开关灯，它至少会以微弱的亮度亮起，不会“假死”。
        if (Brightness > 10) 
        {
            Brightness_Set = Brightness; // 更新记忆值
        }
        
        PWM_SetCompare3(Brightness);
		OLED_ShowString(1,5,"ON ");
		OLED_ShowNum(2,5,Brightness,3);
    }
}

void LED_Init(void)
{
	PWM_Init();
}

void LED1_ON(void)
{
	Brightness = Brightness_Set;
	PWM_SetCompare3(Brightness);
	OLED_ShowString(1,5,"ON ");
	OLED_ShowNum(2,5,Brightness,3);
}

void LED1_OFF(void)
{
	Brightness = 0;
	PWM_SetCompare3(Brightness);
	OLED_ShowString(1,5,"OFF");
	OLED_ShowNum(2,5,Brightness,3);
}
void LED1_Turn(void)
{
	if (Brightness == 0)
	{
		Brightness = Brightness_Set;
		PWM_SetCompare3(Brightness);
		OLED_ShowString(1,5,"ON ");
		OLED_ShowNum(2,5,Brightness,3);
	}
	else
	{
		Brightness = 0;
		PWM_SetCompare3(Brightness);
		OLED_ShowString(1,5,"OFF");
		OLED_ShowNum(2,5,Brightness,3);
	}
}

