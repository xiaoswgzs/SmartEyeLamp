#include "OLED_UI.h"
#include "OLED_GFX.h"

#define OLEDUI_FRAME_MS          80
#define OLEDUI_GESTURE_HOLD_MS  560
#define OLEDUI_DIM_HOLD_MS      780
#define OLEDUI_MODE_HOLD_MS    1050

typedef struct
{
    OLEDUI_Scene_t scene;
    OLEDUI_Scene_t homeScene;

    uint8_t autoMode;
    uint8_t human;
    uint8_t lightOn;
    uint8_t brightness;
    uint8_t targetBrightness;

    uint8_t frame;
    uint32_t lastFrameTime;
    uint32_t sceneStartTime;
    uint32_t tempUntilTime;
} OLEDUI_State_t;

static OLEDUI_State_t s_ui;

static uint8_t UI_Triangle(uint8_t frame, uint8_t max)
{
    uint8_t v;

    v = frame % (max * 2);
    if (v > max)
    {
        v = (uint8_t)(max * 2 - v);
    }
    return v;
}

static void UI_FillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t color)
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

static void UI_DrawStar(uint8_t x, uint8_t y)
{
    OLEDGFX_DrawPixel(x, y, 1);
    OLEDGFX_DrawPixel((uint8_t)(x - 1), y, 1);
    OLEDGFX_DrawPixel((uint8_t)(x + 1), y, 1);
    OLEDGFX_DrawPixel(x, (uint8_t)(y - 1), 1);
    OLEDGFX_DrawPixel(x, (uint8_t)(y + 1), 1);
}

static void UI_DrawModeTag(void)
{
    if (s_ui.autoMode)
    {
        OLEDGFX_ShowString8x16(0, 48, "AUTO");
        if (s_ui.human)
        {
            UI_FillCircle(43, 55, 2, 1);
            OLEDGFX_ShowString8x16(50, 48, "HUMAN");
        }
        else
        {
            OLEDGFX_DrawCircle(43, 55, 2, 1);
            OLEDGFX_ShowString8x16(50, 48, "EMPTY");
        }
    }
    else
    {
        OLEDGFX_ShowString8x16(0, 48, "MANUAL");
        OLEDGFX_ShowString8x16(72, 48, "TOF");
    }
}

static void UI_DrawBoot(uint8_t frame)
{
    uint8_t w;
    uint8_t x;

    OLEDGFX_ShowString8x16(28, 10, "CodeCat");
    OLEDGFX_ShowString8x16(36, 30, "Nite");

    w = (uint8_t)(frame * 5);
    if (w > 92)
    {
        w = 92;
    }

    OLEDGFX_DrawRect(18, 54, 94, 5, 1);
    OLEDGFX_FillRect(19, 55, w, 3, 1);

    x = (uint8_t)(18 + (frame * 7) % 92);
    OLEDGFX_DrawVLine(x, 47, 7, 1);
}

static void UI_DrawIdle(uint8_t frame)
{
    uint8_t b;

    b = UI_Triangle(frame, 5);

    UI_FillCircle(64, 25, 10, 1);
    UI_FillCircle(70, 21, 10, 0);
    OLEDGFX_DrawCircle(64, 25, (uint8_t)(16 + b), 1);

    UI_DrawStar(23, 13);
    UI_DrawStar(102, 17);
    OLEDGFX_DrawPixel(38, 39, 1);
    OLEDGFX_DrawPixel(91, 42, 1);

    UI_DrawModeTag();
}

static void UI_DrawLight(uint8_t frame)
{
    uint8_t glow;
    uint8_t w;
    uint8_t i;

    glow = (uint8_t)(8 + UI_Triangle(frame, 6));

    OLEDGFX_DrawCircle(64, 21, glow, 1);
    OLEDGFX_DrawCircle(64, 21, (uint8_t)(glow + 8), 1);
    OLEDGFX_DrawHLine(48, 34, 32, 1);
    OLEDGFX_FillRect(54, 37, 20, 6, 1);
    OLEDGFX_DrawHLine(50, 46, 28, 1);

    for (i = 0; i < 5; i++)
    {
        if (s_ui.brightness >= (uint8_t)((i + 1) * 18))
        {
            OLEDGFX_FillRect((uint8_t)(32 + i * 14), 3, 8, 2, 1);
        }
        else
        {
            OLEDGFX_DrawRect((uint8_t)(32 + i * 14), 3, 8, 2, 1);
        }
    }

    w = s_ui.targetBrightness;
    if (w > 100)
    {
        w = 100;
    }
    OLEDGFX_DrawRect(88, 35, 34, 7, 1);
    OLEDGFX_FillRect(89, 36, (uint8_t)(w / 3), 5, 1);

    UI_DrawModeTag();
}

static void UI_DrawGesture(uint8_t frame)
{
    uint8_t r;

    r = (uint8_t)(8 + (frame % 6) * 5);

    OLEDGFX_ShowString8x16(28, 2, "GESTURE");
    OLEDGFX_DrawCircle(64, 38, r, 1);
    OLEDGFX_DrawCircle(64, 38, (uint8_t)(r + 10), 1);
    OLEDGFX_FillRect(58, 34, 12, 12, 1);
    OLEDGFX_DrawHLine(52, 31, 24, 1);
    OLEDGFX_DrawHLine(54, 29, 20, 1);
}

static void UI_DrawDimming(uint8_t frame)
{
    uint8_t w;
    uint8_t glow;

    w = s_ui.targetBrightness;
    if (w > 100)
    {
        w = 100;
    }

    glow = UI_Triangle(frame, 4);
    OLEDGFX_DrawCircle(64, 14, (uint8_t)(6 + glow), 1);
    UI_FillCircle(64, 14, 3, 1);

    OLEDGFX_ShowString8x16(24, 27, "BRIGHT");
    OLEDGFX_ShowNum8x16(80, 27, w, 3);

    OLEDGFX_DrawRect(12, 49, 104, 9, 1);
    OLEDGFX_FillRect(14, 51, w, 5, 1);
}

static void UI_DrawMode(uint8_t frame)
{
    uint8_t pulse;

    pulse = UI_Triangle(frame, 5);

    OLEDGFX_DrawRect(12, 8, 104, 44, 1);
    OLEDGFX_DrawRect((uint8_t)(16 - pulse / 2), (uint8_t)(12 - pulse / 2), (uint8_t)(96 + pulse), (uint8_t)(36 + pulse), 1);

    if (s_ui.autoMode)
    {
        OLEDGFX_ShowString8x16(36, 17, "AUTO");
        OLEDGFX_ShowString8x16(28, 36, "RADAR ON");
    }
    else
    {
        OLEDGFX_ShowString8x16(28, 17, "MANUAL");
        OLEDGFX_ShowString8x16(28, 36, "TOF CTRL");
    }

    OLEDGFX_DrawHLine(99, 18, 10, 1);
    OLEDGFX_DrawVLine(108, 9, 10, 1);
}

static void UI_UpdateHomeScene(void)
{
    if (s_ui.lightOn)
    {
        s_ui.homeScene = OLEDUI_SCENE_LIGHT;
    }
    else
    {
        s_ui.homeScene = OLEDUI_SCENE_IDLE;
    }
}

static void UI_GotoScene(OLEDUI_Scene_t scene, uint32_t nowMs)
{
    if (s_ui.scene != scene)
    {
        s_ui.scene = scene;
        s_ui.frame = 0;
        s_ui.sceneStartTime = nowMs;
        OLEDGFX_MarkAllDirty();
    }
}

static void UI_Render(void)
{
    OLEDGFX_ClearBuffer();

    switch (s_ui.scene)
    {
        case OLEDUI_SCENE_BOOT:
            UI_DrawBoot(s_ui.frame);
            break;

        case OLEDUI_SCENE_IDLE:
            UI_DrawIdle(s_ui.frame);
            break;

        case OLEDUI_SCENE_LIGHT:
            UI_DrawLight(s_ui.frame);
            break;

        case OLEDUI_SCENE_GESTURE:
            UI_DrawGesture(s_ui.frame);
            break;

        case OLEDUI_SCENE_DIMMING:
            UI_DrawDimming(s_ui.frame);
            break;

        case OLEDUI_SCENE_MODE:
            UI_DrawMode(s_ui.frame);
            break;

        default:
            UI_DrawIdle(s_ui.frame);
            break;
    }
}

void OLEDUI_Init(void)
{
    s_ui.scene = OLEDUI_SCENE_BOOT;
    s_ui.homeScene = OLEDUI_SCENE_IDLE;
    s_ui.autoMode = 0;
    s_ui.human = 0;
    s_ui.lightOn = 0;
    s_ui.brightness = 0;
    s_ui.targetBrightness = 0;
    s_ui.frame = 0;
    s_ui.lastFrameTime = 0;
    s_ui.sceneStartTime = 0;
    s_ui.tempUntilTime = 0;

    OLEDGFX_ClearBuffer();
    UI_Render();
    OLEDGFX_UpdateAll();
}

void OLEDUI_SetLight(uint8_t isOn, uint8_t brightness)
{
    s_ui.lightOn = isOn ? 1 : 0;
    s_ui.brightness = brightness;
    UI_UpdateHomeScene();
}

void OLEDUI_SetBrightness(uint8_t brightness, uint8_t targetBrightness)
{
    s_ui.brightness = brightness;
    s_ui.targetBrightness = targetBrightness;
}

void OLEDUI_SetMode(uint8_t autoMode)
{
    s_ui.autoMode = autoMode ? 1 : 0;
}

void OLEDUI_SetHuman(uint8_t human)
{
    s_ui.human = human ? 1 : 0;
}

void OLEDUI_ShowGesture(uint32_t nowMs)
{
    s_ui.tempUntilTime = nowMs + OLEDUI_GESTURE_HOLD_MS;
    UI_GotoScene(OLEDUI_SCENE_GESTURE, nowMs);
}

void OLEDUI_ShowDimming(uint32_t nowMs)
{
    s_ui.tempUntilTime = nowMs + OLEDUI_DIM_HOLD_MS;
    UI_GotoScene(OLEDUI_SCENE_DIMMING, nowMs);
}

void OLEDUI_ShowLightState(uint32_t nowMs)
{
    s_ui.tempUntilTime = 0;
    UI_UpdateHomeScene();
    UI_GotoScene(s_ui.homeScene, nowMs);
}

void OLEDUI_ShowMode(uint8_t autoMode, uint32_t nowMs)
{
    s_ui.autoMode = autoMode ? 1 : 0;
    s_ui.tempUntilTime = nowMs + OLEDUI_MODE_HOLD_MS;
    UI_GotoScene(OLEDUI_SCENE_MODE, nowMs);
}

void OLEDUI_Task(uint32_t nowMs)
{
    OLEDGFX_UpdateOnePage();

    if ((uint32_t)(nowMs - s_ui.lastFrameTime) < OLEDUI_FRAME_MS)
    {
        return;
    }

    s_ui.lastFrameTime = nowMs;
    s_ui.frame++;

    if (s_ui.scene == OLEDUI_SCENE_BOOT && s_ui.frame > 22)
    {
        UI_UpdateHomeScene();
        UI_GotoScene(s_ui.homeScene, nowMs);
    }
    else if (s_ui.tempUntilTime != 0 && (int32_t)(nowMs - s_ui.tempUntilTime) >= 0)
    {
        s_ui.tempUntilTime = 0;
        UI_UpdateHomeScene();
        UI_GotoScene(s_ui.homeScene, nowMs);
    }

    UI_Render();
}
