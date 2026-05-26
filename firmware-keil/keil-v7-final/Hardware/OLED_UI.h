#ifndef __OLED_UI_H
#define __OLED_UI_H

#include "stm32f10x.h"

typedef enum
{
    OLEDUI_SCENE_BOOT = 0,
    OLEDUI_SCENE_IDLE,
    OLEDUI_SCENE_LIGHT,
    OLEDUI_SCENE_GESTURE,
    OLEDUI_SCENE_DIMMING,
    OLEDUI_SCENE_MODE
} OLEDUI_Scene_t;

void OLEDUI_Init(void);
void OLEDUI_Task(uint32_t nowMs);

void OLEDUI_SetLight(uint8_t isOn, uint8_t brightness);
void OLEDUI_SetBrightness(uint8_t brightness, uint8_t targetBrightness);
void OLEDUI_SetMode(uint8_t autoMode);
void OLEDUI_SetHuman(uint8_t human);

void OLEDUI_ShowGesture(uint32_t nowMs);
void OLEDUI_ShowDimming(uint32_t nowMs);
void OLEDUI_ShowLightState(uint32_t nowMs);
void OLEDUI_ShowMode(uint8_t autoMode, uint32_t nowMs);

#endif
