#include "OLED_UI.h"
#include "OLED_GFX.h"

#define OLEDUI_FRAME_MS          60
#define OLEDUI_GESTURE_HOLD_MS  650
#define OLEDUI_DIM_HOLD_MS      900
#define OLEDUI_LIGHT_HOLD_MS    900

typedef struct
{
    OLEDUI_Scene_t Scene;
    OLEDUI_Scene_t HomeScene;

    uint8_t LightOn;
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

static uint8_t OLEDUI_TriangleWave(uint8_t frame, uint8_t max)
{
    uint8_t v;

    v = frame % (max * 2);
    if (v > max)
    {
        v = max * 2 - v;
    }

    return v;
}

static void OLEDUI_FillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t color)
{
    int16_t x;
    int16_t y;
    int16_t rr;

    rr = r * r;

    for (y = -r; y <= r; y++)
    {
        for (x = -r; x <= r; x++)
        {
            if (x * x + y * y <= rr)
            {
                OLEDGFX_DrawPixel((uint8_t)(x0 + x), (uint8_t)(y0 + y), color);
            }
        }
    }
}

static void OLEDUI_DrawSpark(uint8_t x, uint8_t y)
{
    OLEDGFX_DrawPixel(x, y, 1);
    OLEDGFX_DrawPixel(x - 1, y, 1);
    OLEDGFX_DrawPixel(x + 1, y, 1);
    OLEDGFX_DrawPixel(x, y - 1, 1);
    OLEDGFX_DrawPixel(x, y + 1, 1);
}

static void OLEDUI_DrawLogo(uint8_t frame)
{
    uint8_t w;

    OLEDGFX_ShowString8x16(32, 13, "CodeCat");
    OLEDGFX_ShowString8x16(40, 33, "Light");

    w = 12 + frame * 4;
    if (w > 84)
    {
        w = 84;
    }

    OLEDGFX_DrawRect(21, 54, 86, 5, 1);
    OLEDGFX_FillRect(22, 55, w, 3, 1);
}

static void OLEDUI_DrawIdle(uint8_t frame)
{
    uint8_t breathe;

    breathe = OLEDUI_TriangleWave(frame, 6);

    OLEDUI_FillCircle(62, 31, 10, 1);
    OLEDUI_FillCircle(67, 27, 10, 0);

    OLEDGFX_DrawCircle(62, 31, 15 + breathe / 2, 1);

    OLEDUI_DrawSpark(24, 15);
    OLEDUI_DrawSpark(101, 18);
    OLEDGFX_DrawPixel(35, 45, 1);
    OLEDGFX_DrawPixel(95, 49, 1);
}

static void OLEDUI_DrawLightOn(uint8_t frame)
{
    uint8_t glow;
    uint8_t i;

    glow = 10 + OLEDUI_TriangleWave(frame, 7);

    OLEDGFX_DrawCircle(64, 25, glow, 1);
    OLEDGFX_DrawCircle(64, 25, glow + 7, 1);
    OLEDGFX_DrawCircle(64, 25, glow + 14, 1);

    OLEDGFX_FillRect(52, 43, 24, 5, 1);
    OLEDGFX_DrawHLine(46, 52, 36, 1);
    OLEDGFX_DrawHLine(50, 56, 28, 1);

    for (i = 0; i < UI.Brightness / 20; i++)
    {
        OLEDGFX_FillRect(35 + i * 12, 61, 6, 2, 1);
    }
}

static void OLEDUI_DrawGesture(uint8_t frame)
{
    uint8_t r;

    r = 8 + (frame % 8) * 4;

    OLEDGFX_ShowString8x16(40, 2, "WAVE");
    OLEDGFX_DrawCircle(64, 39, r, 1);
    OLEDGFX_DrawCircle(64, 39, r + 8, 1);
    OLEDGFX_DrawCircle(64, 39, r + 16, 1);

    OLEDGFX_FillRect(58, 34, 12, 10, 1);
    OLEDGFX_FillRect(55, 30, 18, 5, 1);
}

static void OLEDUI_DrawDimming(uint8_t frame)
{
    uint8_t w;
    uint8_t glow;

    w = UI.TargetBrightness;
    if (w > 100)
    {
        w = 100;
    }

    glow = OLEDUI_TriangleWave(frame, 5);

    OLEDGFX_DrawCircle(64, 17, 7 + glow, 1);
    OLEDUI_FillCircle(64, 17, 4, 1);

    OLEDGFX_DrawRect(13, 39, 102, 10, 1);
    OLEDGFX_FillRect(14, 40, w, 8, 1);

    OLEDGFX_ShowString8x16(44, 53, "BRT");
    OLEDGFX_ShowNum8x16(76, 53, w, 3);
}

static void OLEDUI_DrawDebug(void)
{
    OLEDGFX_ShowString8x16(0, 0, "BRT:");
    OLEDGFX_ShowNum8x16(32, 0, UI.Brightness, 3);

    OLEDGFX_ShowString8x16(0, 20, "DST:");
    OLEDGFX_ShowNum8x16(32, 20, UI.DistanceMm, 4);

    OLEDGFX_ShowString8x16(0, 40, UI.LightOn ? "LED:ON " : "LED:OFF");
}

static void OLEDUI_GotoScene(OLEDUI_Scene_t scene, uint32_t nowMs)
{
    if (UI.Scene != scene)
    {
        UI.Scene = scene;
        UI.Frame = 0;
        UI.SceneStartTime = nowMs;
        OLEDGFX_MarkAllDirty();
    }
}

static void OLEDUI_UpdateHomeScene(void)
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

static void OLEDUI_Render(void)
{
    OLEDGFX_ClearBuffer();

    if (UI.DebugMode)
    {
        OLEDUI_DrawDebug();
        return;
    }

    switch (UI.Scene)
    {
        case OLEDUI_SCENE_BOOT:
            OLEDUI_DrawLogo(UI.Frame);
            break;

        case OLEDUI_SCENE_IDLE:
            OLEDUI_DrawIdle(UI.Frame);
            break;

        case OLEDUI_SCENE_LIGHT_ON:
            OLEDUI_DrawLightOn(UI.Frame);
            break;

        case OLEDUI_SCENE_DIMMING:
            OLEDUI_DrawDimming(UI.Frame);
            break;

        case OLEDUI_SCENE_GESTURE:
            OLEDUI_DrawGesture(UI.Frame);
            break;

        case OLEDUI_SCENE_DEBUG:
            OLEDUI_DrawDebug();
            break;

        default:
            OLEDUI_DrawIdle(UI.Frame);
            break;
    }
}

void OLEDUI_Init(void)
{
    UI.Scene = OLEDUI_SCENE_BOOT;
    UI.HomeScene = OLEDUI_SCENE_IDLE;
    UI.LightOn = 0;
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
    OLEDUI_Render();
    OLEDGFX_UpdateAll();
}

void OLEDUI_SetScene(OLEDUI_Scene_t scene)
{
    OLEDUI_GotoScene(scene, UI.LastFrameTime);
}

void OLEDUI_SetLight(uint8_t isOn, uint8_t brightness)
{
    UI.LightOn = isOn ? 1 : 0;
    UI.Brightness = brightness;
    OLEDUI_UpdateHomeScene();
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

void OLEDUI_ShowGesture(uint32_t nowMs)
{
    UI.TempUntilTime = nowMs + OLEDUI_GESTURE_HOLD_MS;
    OLEDUI_GotoScene(OLEDUI_SCENE_GESTURE, nowMs);
}

void OLEDUI_ShowDimming(uint32_t nowMs)
{
    UI.TempUntilTime = nowMs + OLEDUI_DIM_HOLD_MS;
    OLEDUI_GotoScene(OLEDUI_SCENE_DIMMING, nowMs);
}

void OLEDUI_ShowLightState(uint32_t nowMs)
{
    UI.TempUntilTime = nowMs + OLEDUI_LIGHT_HOLD_MS;
    OLEDUI_UpdateHomeScene();
    OLEDUI_GotoScene(UI.HomeScene, nowMs);
}

void OLEDUI_SetDebug(uint8_t enable)
{
    UI.DebugMode = enable ? 1 : 0;
    OLEDGFX_MarkAllDirty();
}

void OLEDUI_Task(uint32_t nowMs)
{
    OLEDGFX_UpdateOnePage();

    if ((uint32_t)(nowMs - UI.LastFrameTime) < OLEDUI_FRAME_MS)
    {
        return;
    }

    UI.LastFrameTime = nowMs;
    UI.Frame++;

    if (!UI.DebugMode)
    {
        if (UI.Scene == OLEDUI_SCENE_BOOT && UI.Frame > 28)
        {
            OLEDUI_UpdateHomeScene();
            OLEDUI_GotoScene(UI.HomeScene, nowMs);
        }
        else if (UI.TempUntilTime != 0 && (int32_t)(nowMs - UI.TempUntilTime) >= 0)
        {
            UI.TempUntilTime = 0;
            OLEDUI_UpdateHomeScene();
            OLEDUI_GotoScene(UI.HomeScene, nowMs);
        }
    }

    OLEDUI_Render();
}
