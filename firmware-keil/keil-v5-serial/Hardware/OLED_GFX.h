#ifndef __OLED_GFX_H
#define __OLED_GFX_H

#include "stm32f10x.h"

#define OLED_WIDTH   128
#define OLED_HEIGHT   64
#define OLED_PAGES     8

void OLEDGFX_ClearBuffer(void);
void OLEDGFX_FillBuffer(uint8_t color);
void OLEDGFX_UpdateAll(void);
void OLEDGFX_UpdateOnePage(void);
void OLEDGFX_MarkAllDirty(void);

void OLEDGFX_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLEDGFX_DrawHLine(uint8_t x, uint8_t y, uint8_t w, uint8_t color);
void OLEDGFX_DrawVLine(uint8_t x, uint8_t y, uint8_t h, uint8_t color);
void OLEDGFX_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLEDGFX_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLEDGFX_DrawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t color);
void OLEDGFX_DrawBitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap, uint8_t color);

void OLEDGFX_ShowChar8x16(uint8_t x, uint8_t y, char ch);
void OLEDGFX_ShowString8x16(uint8_t x, uint8_t y, const char *str);
void OLEDGFX_ShowNum8x16(uint8_t x, uint8_t y, uint32_t num, uint8_t len);

#endif
