#include "App.h"
#include "SystemTime.h"
#include "Touch.h"
#include "LED.h"
#include "LD2410B.h"
#include "My_VL53L0X.h"
#include "Gesture_ToF.h"
#include "OLED_UI.h"
#include "Serial.h"
#include <stdio.h>
#include <string.h>

#define RADAR_SCAN_MS        70
#define GESTURE_SCAN_MS      30
#define GESTURE_DIM_MS       18
#define STATUS_SYNC_MS       80
#define PANEL_SYNC_MS      1000

#define TOUCH_BRIGHT_STEP     6
#define GESTURE_DIM_STEP      5

typedef struct
{
    AppMode_t mode;
    uint8_t lastHuman;
    uint8_t gestureDimUp;

    uint32_t radarTick;
    uint32_t gestureTick;
    uint32_t gestureDimTick;
    uint32_t statusTick;
    uint32_t panelTick;

    uint8_t panelLastTarget;
    uint8_t panelLastLed;
    uint8_t panelLastMode;
    uint8_t panelLastHuman;
} AppState_t;

static AppState_t s_app;

static void App_SendModeText(void)
{
    if (s_app.mode == APP_MODE_AUTO)
    {
        Serial_SendString("MODE_AUTO\r\n");
        printf("[d,1,21,模式：自动]");
    }
    else
    {
        Serial_SendString("MODE_MANUAL\r\n");
        printf("[d,1,21,模式：手动]");
    }
}

static void App_ApplyAutoLight(void)
{
    if (s_app.mode != APP_MODE_AUTO)
    {
        return;
    }

    if (s_app.lastHuman)
    {
        LED1_ON();
    }
    else
    {
        LED1_OFF();
    }
}

static void App_SetMode(AppMode_t mode, uint32_t nowMs)
{
    s_app.mode = mode;
    s_app.lastHuman = LD2410B_GetState();

    OLEDUI_SetMode(s_app.mode == APP_MODE_AUTO);
    OLEDUI_SetHuman(s_app.lastHuman);
    OLEDUI_ShowMode(s_app.mode == APP_MODE_AUTO, nowMs);

    LED1_AckFlash(2);
    App_SendModeText();

    if (s_app.mode == APP_MODE_AUTO)
    {
        App_ApplyAutoLight();
    }
}

static void App_ToggleMode(uint32_t nowMs)
{
    if (s_app.mode == APP_MODE_AUTO)
    {
        App_SetMode(APP_MODE_MANUAL, nowMs);
    }
    else
    {
        App_SetMode(APP_MODE_AUTO, nowMs);
    }
}

static void App_AdjustBrightness(int8_t step, uint32_t nowMs)
{
    if (s_app.mode == APP_MODE_AUTO && !s_app.lastHuman)
    {
        LED1_AddMemory(step);
        OLEDUI_SetBrightness(LED1_GetBrightness(), Get_Brightness_Set());
        OLEDUI_ShowDimming(nowMs);
        return;
    }

    if (!LED1_IsOn())
    {
        LED1_ON();
    }

    LED1_AddTarget(step);
    OLEDUI_ShowDimming(nowMs);
}

static void App_HandleTouch(uint32_t nowMs)
{
    TouchEvent_t event;

    Touch_Task(nowMs);
    event = Touch_TakeEvent();

    switch (event)
    {
        case TOUCH_EVENT_MODE:
            App_ToggleMode(nowMs);
            break;

        case TOUCH_EVENT_BRIGHT_UP:
            App_AdjustBrightness(TOUCH_BRIGHT_STEP, nowMs);
            break;

        case TOUCH_EVENT_BRIGHT_DOWN:
            App_AdjustBrightness(-TOUCH_BRIGHT_STEP, nowMs);
            break;

        default:
            break;
    }
}

static void App_HandleRadar(uint32_t nowMs)
{
    uint8_t human;

    if (!SystemTime_IsDue(&s_app.radarTick, RADAR_SCAN_MS))
    {
        return;
    }

    human = LD2410B_GetState();
    if (human != s_app.lastHuman)
    {
        s_app.lastHuman = human;
        OLEDUI_SetHuman(human);
        printf(human ? "[d,1,41,人体：有人]" : "[d,1,41,人体：无人]");
    }

    if (s_app.mode == APP_MODE_AUTO)
    {
        App_ApplyAutoLight();
        OLEDUI_ShowLightState(nowMs);
    }
}

static void App_GestureDimming(uint32_t nowMs)
{
    uint8_t target;

    if ((uint32_t)(nowMs - s_app.gestureDimTick) < GESTURE_DIM_MS)
    {
        return;
    }

    s_app.gestureDimTick = nowMs;

    if (s_app.mode == APP_MODE_AUTO && !s_app.lastHuman)
    {
        target = Get_Brightness_Set();
    }
    else
    {
        target = LED1_GetTarget();
    }

    if (s_app.gestureDimUp)
    {
        if (target >= 96)
        {
            s_app.gestureDimUp = 0;
            App_AdjustBrightness(-GESTURE_DIM_STEP, nowMs);
        }
        else
        {
            App_AdjustBrightness(GESTURE_DIM_STEP, nowMs);
        }
    }
    else
    {
        if (target <= 14)
        {
            s_app.gestureDimUp = 1;
            App_AdjustBrightness(GESTURE_DIM_STEP, nowMs);
        }
        else
        {
            App_AdjustBrightness(-GESTURE_DIM_STEP, nowMs);
        }
    }
}

static void App_HandleGesture(uint32_t nowMs)
{
    uint16_t distance;
    GestureEvent_t event;

    if (!SystemTime_IsDue(&s_app.gestureTick, GESTURE_SCAN_MS))
    {
        return;
    }

    distance = MyVL53L0X_GetDistance();
    event = Gesture_Update(distance, nowMs);

    switch (event)
    {
        case GESTURE_EVENT_NEAR:
            OLEDUI_ShowGesture(nowMs);
            break;

        case GESTURE_EVENT_WAVE:
            if (s_app.mode == APP_MODE_MANUAL)
            {
                LED1_Turn();
                OLEDUI_ShowGesture(nowMs);
                Serial_SendString("GESTURE_TOGGLE\r\n");
            }
            else
            {
                OLEDUI_ShowGesture(nowMs);
                LED1_AckFlash(1);
                Serial_SendString("GESTURE_AUTO\r\n");
            }
            break;

        case GESTURE_EVENT_HOLD_START:
            s_app.gestureDimTick = nowMs;
            App_AdjustBrightness(0, nowMs);
            Serial_SendString("GESTURE_DIM_START\r\n");
            break;

        case GESTURE_EVENT_HOLDING:
            App_GestureDimming(nowMs);
            break;

        case GESTURE_EVENT_HOLD_END:
            s_app.gestureDimUp = !s_app.gestureDimUp;
            OLEDUI_ShowLightState(nowMs);
            Serial_SendString("GESTURE_DIM_END\r\n");
            break;

        default:
            break;
    }
}

static void App_HandleSerial(uint32_t nowMs)
{
    if (Serial_RXFlag == 0)
    {
        return;
    }

    if (strcmp(Serial_RXPacket, "MODE_AUTO") == 0)
    {
        App_SetMode(APP_MODE_AUTO, nowMs);
    }
    else if (strcmp(Serial_RXPacket, "MODE_MANUAL") == 0)
    {
        App_SetMode(APP_MODE_MANUAL, nowMs);
    }
    else if (strcmp(Serial_RXPacket, "MODE_TOGGLE") == 0)
    {
        App_ToggleMode(nowMs);
    }
    else if (strcmp(Serial_RXPacket, "LED_ON") == 0)
    {
        if (s_app.mode != APP_MODE_AUTO || s_app.lastHuman)
        {
            LED1_ON();
        }
        OLEDUI_ShowLightState(nowMs);
        Serial_SendString("LED_ON_OK\r\n");
    }
    else if (strcmp(Serial_RXPacket, "LED_OFF") == 0)
    {
        if (s_app.mode != APP_MODE_AUTO)
        {
            LED1_OFF();
        }
        OLEDUI_ShowLightState(nowMs);
        Serial_SendString("LED_OFF_OK\r\n");
    }
    else if (strcmp(Serial_RXPacket, "BRTI") == 0)
    {
        App_AdjustBrightness(10, nowMs);
        Serial_SendString("BRTI_OK\r\n");
    }
    else if (strcmp(Serial_RXPacket, "BRTD") == 0)
    {
        App_AdjustBrightness(-10, nowMs);
        Serial_SendString("BRTD_OK\r\n");
    }
    else
    {
        Serial_SendString("Command_Error\r\n");
    }

    Serial_RXFlag = 0;
}

static void App_SyncStatus(void)
{
    uint8_t uiTarget;

    if (!SystemTime_IsDue(&s_app.statusTick, STATUS_SYNC_MS))
    {
        return;
    }

    if (s_app.mode == APP_MODE_AUTO && !s_app.lastHuman)
    {
        uiTarget = Get_Brightness_Set();
    }
    else
    {
        uiTarget = LED1_GetTarget();
    }

    OLEDUI_SetMode(s_app.mode == APP_MODE_AUTO);
    OLEDUI_SetHuman(s_app.lastHuman);
    OLEDUI_SetLight(LED1_IsOn(), LED1_GetBrightness());
    OLEDUI_SetBrightness(LED1_GetBrightness(), uiTarget);
}

static void App_SyncPanel(void)
{
    uint8_t target;
    uint8_t led;
    uint8_t mode;
    uint8_t human;

    if (!SystemTime_IsDue(&s_app.panelTick, PANEL_SYNC_MS))
    {
        return;
    }

    target = Get_Brightness_Set();
    led = LED1_IsOn();
    mode = (uint8_t)s_app.mode;
    human = s_app.lastHuman;

    if (target != s_app.panelLastTarget)
    {
        s_app.panelLastTarget = target;
        printf("[d,1,61,亮度记忆：%03d]", target);
    }

    if (led != s_app.panelLastLed)
    {
        s_app.panelLastLed = led;
        printf(led ? "[d,1,81,灯光：开]" : "[d,1,81,灯光：关]");
    }

    if (mode != s_app.panelLastMode)
    {
        s_app.panelLastMode = mode;
        printf(mode ? "[d,1,21,模式：自动]" : "[d,1,21,模式：手动]");
    }

    if (human != s_app.panelLastHuman)
    {
        s_app.panelLastHuman = human;
        printf(human ? "[d,1,41,人体：有人]" : "[d,1,41,人体：无人]");
    }
}

void App_Init(void)
{
    s_app.mode = APP_MODE_MANUAL;
    s_app.lastHuman = LD2410B_GetState();
    s_app.gestureDimUp = 1;

    s_app.radarTick = 0;
    s_app.gestureTick = 0;
    s_app.gestureDimTick = 0;
    s_app.statusTick = 0;
    s_app.panelTick = 0;

    s_app.panelLastTarget = 0xFF;
    s_app.panelLastLed = 0xFF;
    s_app.panelLastMode = 0xFF;
    s_app.panelLastHuman = 0xFF;

    Gesture_Init();
    OLEDUI_SetMode(0);
    OLEDUI_SetHuman(s_app.lastHuman);
    OLEDUI_ShowMode(0, SystemTime_Millis());
    App_SendModeText();
}

void App_Task(void)
{
    uint32_t now;

    now = SystemTime_Millis();

    App_HandleTouch(now);
    App_HandleGesture(now);
    App_HandleSerial(now);
    App_HandleRadar(now);

    LED1_Task(now);
    App_SyncStatus();
    App_SyncPanel();
    OLEDUI_Task(now);
}
