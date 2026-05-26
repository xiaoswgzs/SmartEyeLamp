#ifndef __OLED_FONT_H
#define __OLED_FONT_H

#include "stm32f10x.h"

/*
 * 字体表只在 OLED_Font.c 里定义一次。
 * 其他 .c 文件通过 extern 使用它，避免 Keil 链接时报 multiply defined。
 */
extern const uint8_t OLED_F8x16[][16];

#endif
