#include "stm32f10x.h"
#include "SystemTime.h"
#include "OLED.H"
#include "OLED_UI.h"
#include "Key.h"
#include "Serial.h"
#include "LED.h"
#include "LD2410B.h"
#include "My_VL53L0X.h"
#include "Touch.h"
#include "App.h"
#include <stdio.h>

static void Board_Init(void)
{
    SystemTime_Init();

    OLED_Init();
    OLEDUI_Init();

    Key_Init();
    Serial_Init();
    LED_Init();
    LD2410B_Init();
    MyVL53L0X_Init();

    Touch_Init();
    App_Init();

    printf("[d,1,21,模式：手动]");
    printf("[d,1,61,极简护眼小夜灯]");
    printf("[d,1,111,      CodeCat Studio]");
}

int main(void)
{
    Board_Init();

    while (1)
    {
        App_Task();
    }
}
