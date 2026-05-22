#include "OLED_UI.h"
#include "OLED_GFX.h"

#define OLEDUI_FRAME_MS          75
#define OLEDUI_FLUSH_PAGES        2

#define OLEDUI_GESTURE_HOLD_MS  520
#define OLEDUI_DIM_HOLD_MS      850
#define OLEDUI_LIGHT_HOLD_MS    650
#define OLEDUI_MODE_HOLD_MS    1150

typedef struct
{
    OLEDUI_Scene_t Scene;
    OLEDUI_Scene_t HomeScene;

    uint8_t LightOn;
    uint8_t AutoMode;
    uint8_t Human;
    uint8_t Brightness;
    uint8_t TargetBrightness;
    uint8_t GestureLevel;
    uint8_t DebugMode;
    uint16_t DistanceMm;

    uint8_t Frame;
    uint32_t LastFrameTime;
    uint32_t SceneStartTime;
    uint32_t TempUntilTime;
} OLEDUI_State_t;

static OLEDUI_State_t UI;

static uint8_t UI_Tri(uint8_t frame, uint8_t max)
{
    uint8_t v;

    if (max == 0)
    {
        return 0;
    }

    v = frame % (max * 2);
    if (v > max)
    {
        v = max * 2 - v;
    }

    return v;
}

static void UI_Pixel(int16_t x, int16_t y, uint8_t color)
{
    if (x < 0 || y < 0 || x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    OLEDGFX_DrawPixel((uint8_t)x, (uint8_t)y, color);
}

static void UI_FillCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t color)
{
    int16_t x;
    int16_t y;
    int16_t rr;

    rr = r * r;

    for (y = -r; y <= r; y++)
    {
        for (x = -r; x <= r; x++)
        {
            if ((x * x + y * y) <= rr)
            {
                UI_Pixel(x0 + x, y0 + y, color);
            }
        }
    }
}

static void UI_Spark(uint8_t x, uint8_t y)
{
    UI_Pixel(x, y, 1);
    UI_Pixel(x - 1, y, 1);
    UI_Pixel(x + 1, y, 1);
    UI_Pixel(x, y - 1, 1);
    UI_Pixel(x, y + 1, 1);
}

static void UI_Header(void)
{
    if (UI.AutoMode)
    {
        OLEDGFX_ShowString8x16(0, 0, "AUTO");
    }
    else
    {
        OLEDGFX_ShowString8x16(0, 0, "MAN ");
    }

    OLEDGFX_ShowString8x16(40, 0, UI.Human ? "H" : "-");
    OLEDGFX_ShowString8x16(56, 0, "B");
    OLEDGFX_ShowNum8x16(72, 0, UI.TargetBrightness, 3);
}

static void UI_DrawLogo(uint8_t frame)
{
    uint8_t w;

    OLEDGFX_ShowString8x16(28, 10, "CodeCat");
    OLEDGFX_ShowString8x16(44, 30, "Glow");

    w = 12 + frame * 4;
    if (w > 84)
    {
        w = 84;
    }

    OLEDGFX_DrawRect(21, 54, 86, 5, 1);
    OLEDGFX_FillRect(22, 55, w, 3, 1);
}

static void UI_DrawIdle(uint8_t frame)
{
    uint8_t breathe;
    uint8_t i;

    breathe = UI_Tri(frame, 8);
    UI_Header();

    UI_FillCircle(64, 36, 11, 1);
    UI_FillCircle(70, 31, 12, 0);
    OLEDGFX_DrawCircle(64, 36, 17 + breathe / 3, 1);

    UI_Spark(25, 25);
    UI_Spark(102, 22);
    UI_Pixel(34, 51, 1);
    UI_Pixel(96, 52, 1);

    for (i = 0; i < 4; i++)
    {
        if (((frame + i) & 0x03) < 2)
        {
            OLEDGFX_FillRect(48 + i * 10, 59, 4, 2, 1);
        }
    }
}

static void UI_DrawLightOn(uint8_t frame)
{
    uint8_t glow;
    uint8_t rays;
    uint8_t bars;
    uint8_t i;

    glow = 9 + UI_Tri(frame, 6);
    rays = frame & 0x03;
    bars = UI.TargetBrightness / 12;
    if (bars > 8)
    {
        bars = 8;
    }

    UI_Header();

    UI_FillCircle(64, 34, 7, 1);
    OLEDGFX_DrawCircle(64, 34, glow, 1);
    OLEDGFX_DrawCircle(64, 34, glow + 8, 1);

    OLEDGFX_DrawVLine(64, 19 - rays, 6, 1);
    OLEDGFX_DrawVLine(64, 48, 5 + rays, 1);
    OLEDGFX_DrawHLine(44 - rays, 34, 7, 1);
    OLEDGFX_DrawHLine(77, 34, 7 + rays, 1);

    OLEDGFX_DrawRect(18, 56, 92, 6, 1);
    for (i = 0; i < bars; i++)
    {
        OLEDGFX_FillRect(21 + i * 11, 58, 7, 2, 1);
    }
}

static void UI_DrawGesture(uint8_t frame)
{
    uint8_t r;

    r = 9 + (frame % 6) * 5;

    OLEDGFX_ShowString8x16(28, 0, "GESTURE");
    OLEDGFX_DrawCircle(64, 37, r, 1);
    OLEDGFX_DrawCircle(64, 37, r + 8, 1);
    OLEDGFX_DrawCircle(64, 37, r + 16, 1);

    OLEDGFX_FillRect(58, 34, 12, 12, 1);
    OLEDGFX_FillRect(55, 29, 18, 6, 1);
    OLEDGFX_FillRect(52, 32, 5, 10, 1);
    OLEDGFX_FillRect(72, 32, 5, 10, 1);
}

static void UI_DrawDimming(uint8_t frame)
{
    uint8_t w;
    uint8_t glow;

    w = UI.TargetBrightness;
    if (w > 100)
    {
        w = 100;
    }

    glow = UI_Tri(frame, 5);

    OLEDGFX_ShowString8x16(32, 0, "DIMMING");
    UI_FillCircle(64, 24, 4, 1);
    OLEDGFX_DrawCircle(64, 24, 8 + glow, 1);

    OLEDGFX_DrawRect(13, 41, 102, 10, 1);
    OLEDGFX_FillRect(14, 42, w, 8, 1);

    OLEDGFX_ShowString8x16(35, 53, "BRT");
    OLEDGFX_ShowNum8x16(67, 53, w, 3);
}

static void UI_DrawMode(uint8_t frame)
{
    uint8_t i;
    uint8_t pulse;

    pulse = UI_Tri(frame, 5);

    if (UI.AutoMode)
    {
        OLEDGFX_ShowString8x16(40, 9, "AUTO");
        OLEDGFX_ShowString8x16(24, 31, "HUMAN ON");
    }
    else
    {
        OLEDGFX_ShowString8x16(28, 9, "MANUAL");
        OLEDGFX_ShowString8x16(20, 31, "GESTURE");
    }

    for (i = 0; i < 3; i++)
    {
        OLEDGFX_DrawCircle(49 + i * 15, 56, 2 + pulse / 2, 1);
        UI_FillCircle(49 + i * 15, 56, 1, 1);
    }
}

static void UI_DrawDebug(void)
{
    OLEDGFX_ShowString8x16(0, 0, UI.AutoMode ? "MODE:AUTO" : "MODE:MAN ");
    OLEDGFX_ShowString8x16(0, 16, UI.Human ? "HUMAN:YES" : "HUMAN:NO ");
    OLEDGFX_ShowString8x16(0, 32, "BRT:");
    OLEDGFX_ShowNum8x16(32, 32, UI.Brightness, 3);
    OLEDGFX_ShowString8x16(0, 48, "DST:");
    OLEDGFX_ShowNum8x16(32, 48, UI.DistanceMm, 4);
}

static void UI_UpdateHomeScene(void)
{
    if (UI.LightOn)
    {
        UI.HomeScene = OLEDUI_SCENE_LIGHT_ON;
    }
    else
    {
        UI.HomeScene = OLEDUI_SCENE_IDLE;
    }
}

static void UI_GotoScene(OLEDUI_Scene_t scene, uint32_t nowMs)
{
    if (UI.Scene != scene)
    {
        UI.Scene = scene;
        UI.Frame = 0;
        UI.SceneStartTime = nowMs;
        OLEDGFX_MarkAllDirty();
    }
}

static void UI_Render(void)
{
    OLEDGFX_ClearBuffer();

    if (UI.DebugMode)
    {
        UI_DrawDebug();
        return;
    }

    switch (UI.Scene)
    {
        case OLEDUI_SCENE_BOOT:
            UI_DrawLogo(UI.Frame);
            break;

        case OLEDUI_SCENE_IDLE:
            UI_DrawIdle(UI.Frame);
            break;

        case OLEDUI_SCENE_LIGHT_ON:
            UI_DrawLightOn(UI.Frame);
            break;

        case OLEDUI_SCENE_DIMMING:
            UI_DrawDimming(UI.Frame);
            break;

        case OLEDUI_SCENE_GESTURE:
            UI_DrawGesture(UI.Frame);
            break;

        case OLEDUI_SCENE_MODE:
            UI_DrawMode(UI.Frame);
            break;

        case OLEDUI_SCENE_DEBUG:
            UI_DrawDebug();
            break;

        default:
            UI_DrawIdle(UI.Frame);
            break;
    }
}

void OLEDUI_Init(void)
{
    UI.Scene = OLEDUI_SCENE_BOOT;
    UI.HomeScene = OLEDUI_SCENE_IDLE;
    UI.LightOn = 0;
    UI.AutoMode = 1;
    UI.Human = 0;
    UI.Brightness = 0;
    UI.TargetBrightness = 0;
    UI.GestureLevel = 0;
    UI.DebugMode = 0;
    UI.DistanceMm = 0;
    UI.Frame = 0;
    UI.LastFrameTime = 0;
    UI.SceneStartTime = 0;
    UI.TempUntilTime = 0;

    OLEDGFX_ClearBuffer();
    UI_Render();
    OLEDGFX_UpdateAll();
}

void OLEDUI_SetScene(OLEDUI_Scene_t scene)
{
    UI_GotoScene(scene, UI.LastFrameTime);
}

void OLEDUI_SetLight(uint8_t isOn, uint8_t brightness)
{
    UI.LightOn = isOn ? 1 : 0;
    UI.Brightness = brightness;
    UI_UpdateHomeScene();
}

void OLEDUI_SetBrightness(uint8_t brightness, uint8_t targetBrightness)
{
    UI.Brightness = brightness;
    UI.TargetBrightness = targetBrightness;
}

void OLEDUI_SetDistance(uint16_t distanceMm)
{
    UI.DistanceMm = distanceMm;
}

void OLEDUI_SetGestureLevel(uint8_t level)
{
    UI.GestureLevel = level;
}

void OLEDUI_SetDebug(uint8_t enable)
{
    UI.DebugMode = enable ? 1 : 0;
    OLEDGFX_MarkAllDirty();
}

void OLEDUI_SetMode(uint8_t autoMode)
{
    UI.AutoMode = autoMode ? 1 : 0;
}

void OLEDUI_SetHuman(uint8_t human)
{
    UI.Human = human ? 1 : 0;
}

void OLEDUI_ShowGesture(uint32_t nowMs)
{
    UI.TempUntilTime = nowMs + OLEDUI_GESTURE_HOLD_MS;
    UI_GotoScene(OLEDUI_SCENE_GESTURE, nowMs);
}

void OLEDUI_ShowDimming(uint32_t nowMs)
{
    UI.TempUntilTime = nowMs + OLEDUI_DIM_HOLD_MS;
    UI_GotoScene(OLEDUI_SCENE_DIMMING, nowMs);
}

void OLEDUI_ShowLightState(uint32_t nowMs)
{
    UI.TempUntilTime = nowMs + OLEDUI_LIGHT_HOLD_MS;
    UI_UpdateHomeScene();
    UI_GotoScene(UI.HomeScene, nowMs);
}

void OLEDUI_ShowMode(uint32_t nowMs)
{
    UI.TempUntilTime = nowMs + OLEDUI_MODE_HOLD_MS;
    UI_GotoScene(OLEDUI_SCENE_MODE, nowMs);
}

void OLEDUI_Task(uint32_t nowMs)
{
    OLEDGFX_UpdatePages(OLEDUI_FLUSH_PAGES);

    if ((uint32_t)(nowMs - UI.LastFrameTime) < OLEDUI_FRAME_MS)
    {
        return;
    }

    UI.LastFrameTime = nowMs;
    UI.Frame++;

    if (!UI.DebugMode)
    {
        if (UI.Scene == OLEDUI_SCENE_BOOT && UI.Frame > 24)
        {
            UI_UpdateHomeScene();
            UI_GotoScene(UI.HomeScene, nowMs);
        }
        else if (UI.TempUntilTime != 0 && (int32_t)(nowMs - UI.TempUntilTime) >= 0)
        {
            UI.TempUntilTime = 0;
            UI_UpdateHomeScene();
            UI_GotoScene(UI.HomeScene, nowMs);
        }
    }

    UI_Render();
}
