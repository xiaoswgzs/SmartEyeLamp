#include "OLED_GFX.h"
#include "OLED_Font.h"
#include <string.h>

/*
 * Frame buffer for 128x64 SSD1306 OLED.
 * One page = 8 vertical pixels x 128 columns.
 */
static uint8_t OLED_Buffer[OLED_PAGES][OLED_WIDTH];
static uint8_t OLED_DirtyPage[OLED_PAGES];
static uint8_t OLED_FlushPage = 0;

/* Low level functions are provided by the old OLED.c. */
void OLED_SetCursor(uint8_t Y, uint8_t X);
void OLED_I2C_Start(void);
void OLED_I2C_SendByte(uint8_t Byte);
void OLED_I2C_Stop(void);

static void OLED_WriteBuffer(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);
    OLED_I2C_SendByte(0x40);

    for (i = 0; i < len; i++)
    {
        OLED_I2C_SendByte(data[i]);
    }

    OLED_I2C_Stop();
}

static void OLED_MarkPageDirty(uint8_t page)
{
    if (page < OLED_PAGES)
    {
        OLED_DirtyPage[page] = 1;
    }
}

void OLEDGFX_MarkAllDirty(void)
{
    uint8_t i;
    for (i = 0; i < OLED_PAGES; i++)
    {
        OLED_DirtyPage[i] = 1;
    }
}

void OLEDGFX_ClearBuffer(void)
{
    memset(OLED_Buffer, 0x00, sizeof(OLED_Buffer));
    OLEDGFX_MarkAllDirty();
}

void OLEDGFX_FillBuffer(uint8_t color)
{
    memset(OLED_Buffer, color ? 0xFF : 0x00, sizeof(OLED_Buffer));
    OLEDGFX_MarkAllDirty();
}

void OLEDGFX_UpdateAll(void)
{
    uint8_t page;

    for (page = 0; page < OLED_PAGES; page++)
    {
        if (OLED_DirtyPage[page])
        {
            OLED_SetCursor(page, 0);
            OLED_WriteBuffer(OLED_Buffer[page], OLED_WIDTH);
            OLED_DirtyPage[page] = 0;
        }
    }
}

void OLEDGFX_UpdateOnePage(void)
{
    uint8_t i;
    uint8_t page;

    for (i = 0; i < OLED_PAGES; i++)
    {
        page = OLED_FlushPage;
        OLED_FlushPage++;
        if (OLED_FlushPage >= OLED_PAGES)
        {
            OLED_FlushPage = 0;
        }

        if (OLED_DirtyPage[page])
        {
            OLED_SetCursor(page, 0);
            OLED_WriteBuffer(OLED_Buffer[page], OLED_WIDTH);
            OLED_DirtyPage[page] = 0;
            return;
        }
    }
}

void OLEDGFX_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    uint8_t page;
    uint8_t mask;

    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    page = y >> 3;
    mask = 1 << (y & 0x07);

    if (color)
    {
        OLED_Buffer[page][x] |= mask;
    }
    else
    {
        OLED_Buffer[page][x] &= (uint8_t)~mask;
    }

    OLED_MarkPageDirty(page);
}

void OLEDGFX_DrawHLine(uint8_t x, uint8_t y, uint8_t w, uint8_t color)
{
    uint8_t i;

    for (i = 0; i < w; i++)
    {
        OLEDGFX_DrawPixel(x + i, y, color);
    }
}

void OLEDGFX_DrawVLine(uint8_t x, uint8_t y, uint8_t h, uint8_t color)
{
    uint8_t i;

    for (i = 0; i < h; i++)
    {
        OLEDGFX_DrawPixel(x, y + i, color);
    }
}

void OLEDGFX_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    if (w == 0 || h == 0)
    {
        return;
    }

    OLEDGFX_DrawHLine(x, y, w, color);
    OLEDGFX_DrawHLine(x, y + h - 1, w, color);
    OLEDGFX_DrawVLine(x, y, h, color);
    OLEDGFX_DrawVLine(x + w - 1, y, h, color);
}

void OLEDGFX_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    uint8_t i;

    for (i = 0; i < h; i++)
    {
        OLEDGFX_DrawHLine(x, y + i, w, color);
    }
}

void OLEDGFX_DrawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t color)
{
    int16_t x = 0;
    int16_t y = r;
    int16_t d = 3 - 2 * r;

    while (x <= y)
    {
        OLEDGFX_DrawPixel(x0 + x, y0 + y, color);
        OLEDGFX_DrawPixel(x0 - x, y0 + y, color);
        OLEDGFX_DrawPixel(x0 + x, y0 - y, color);
        OLEDGFX_DrawPixel(x0 - x, y0 - y, color);
        OLEDGFX_DrawPixel(x0 + y, y0 + x, color);
        OLEDGFX_DrawPixel(x0 - y, y0 + x, color);
        OLEDGFX_DrawPixel(x0 + y, y0 - x, color);
        OLEDGFX_DrawPixel(x0 - y, y0 - x, color);

        if (d < 0)
        {
            d += 4 * x + 6;
        }
        else
        {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void OLEDGFX_DrawBitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap, uint8_t color)
{
    uint8_t ix, iy;
    uint8_t byte;

    for (iy = 0; iy < h; iy++)
    {
        for (ix = 0; ix < w; ix++)
        {
            byte = bitmap[(iy >> 3) * w + ix];
            if (byte & (1 << (iy & 0x07)))
            {
                OLEDGFX_DrawPixel(x + ix, y + iy, color);
            }
        }
    }
}

void OLEDGFX_ShowChar8x16(uint8_t x, uint8_t y, char ch)
{
    uint8_t i, j;
    uint8_t data;
    const uint8_t *font;

    if (ch < ' ' || ch > '~')
    {
        ch = ' ';
    }

    font = OLED_F8x16[ch - ' '];

    for (i = 0; i < 8; i++)
    {
        data = font[i];
        for (j = 0; j < 8; j++)
        {
            OLEDGFX_DrawPixel(x + i, y + j, data & (1 << j));
        }

        data = font[i + 8];
        for (j = 0; j < 8; j++)
        {
            OLEDGFX_DrawPixel(x + i, y + 8 + j, data & (1 << j));
        }
    }
}

void OLEDGFX_ShowString8x16(uint8_t x, uint8_t y, const char *str)
{
    while (*str != '\0')
    {
        OLEDGFX_ShowChar8x16(x, y, *str);
        x += 8;
        str++;

        if (x > OLED_WIDTH - 8)
        {
            break;
        }
    }
}

void OLEDGFX_ShowNum8x16(uint8_t x, uint8_t y, uint32_t num, uint8_t len)
{
    uint8_t i;
    uint32_t div = 1;

    for (i = 1; i < len; i++)
    {
        div *= 10;
    }

    for (i = 0; i < len; i++)
    {
        OLEDGFX_ShowChar8x16(x + i * 8, y, (num / div) % 10 + '0');
        div /= 10;
    }
}
