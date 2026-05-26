#include "App.h"
#include "SystemTime.h"
#include "Touch.h"
#include "LED.h"
#include "LD2410B.h"
#include "My_VL53L0X.h"
#include "Gesture_ToF.h"
#include "OLED_UI.h"
#include "Protocol.h"

#define RADAR_SCAN_MS        70
#define GESTURE_SCAN_MS      30
#define GESTURE_DIM_MS       18
#define STATUS_SYNC_MS       80

#define TOUCH_BRIGHT_STEP     6
#define GESTURE_DIM_STEP      5

typedef struct
{
    AppMode_t mode;
    uint8_t lastHuman;
    uint8_t lastLight;
    uint8_t lastTarget;
    uint8_t gestureDimUp;

    uint32_t humanTriggerCount;
    uint32_t radarTick;
    uint32_t gestureTick;
    uint32_t gestureDimTick;
    uint32_t statusTick;
} AppState_t;

static AppState_t s_app;

static void App_FillStatus(ProtocolStatus_t *status)
{
    if (status == 0)
    {
        return;
    }

    status->modeAuto = (s_app.mode == APP_MODE_AUTO) ? 1 : 0;
    status->lightOn = LED1_IsOn();
    status->brightness = LED1_GetBrightness();
    status->target = LED1_GetTarget();
    status->human = s_app.lastHuman;
    status->humanTriggerCount = s_app.humanTriggerCount;
}

static void App_SendStatus(void)
{
    ProtocolStatus_t status;

    App_FillStatus(&status);
    Protocol_SendStatus(&status);
}

static void App_ApplyAutoLight(uint32_t nowMs)
{
    uint8_t before;
    uint8_t after;

    if (s_app.mode != APP_MODE_AUTO)
    {
        return;
    }

    before = LED1_IsOn();

    if (s_app.lastHuman)
    {
        LED1_ON();
    }
    else
    {
        LED1_OFF();
    }

    after = LED1_IsOn();
    if (after != before)
    {
        OLEDUI_ShowLightState(nowMs);
        Protocol_SendLight(after);
    }
}

static void App_SetMode(AppMode_t mode, uint32_t nowMs)
{
    if (s_app.mode == mode)
    {
        LED1_AckFlash(1);
        Protocol_SendMode(s_app.mode == APP_MODE_AUTO);
        App_SendStatus();
        return;
    }

    s_app.mode = mode;
    s_app.lastHuman = LD2410B_GetState();

    OLEDUI_SetMode(s_app.mode == APP_MODE_AUTO);
    OLEDUI_SetHuman(s_app.lastHuman);
    OLEDUI_ShowMode(s_app.mode == APP_MODE_AUTO, nowMs);

    LED1_AckFlash(2);
    Protocol_SendMode(s_app.mode == APP_MODE_AUTO);

    App_ApplyAutoLight(nowMs);
    App_SendStatus();
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

static void App_SetBrightness(uint8_t level, uint32_t nowMs)
{
    if (s_app.mode == APP_MODE_AUTO && !s_app.lastHuman)
    {
        LED1_SetMemory(level);
        OLEDUI_SetBrightness(LED1_GetBrightness(), Get_Brightness_Set());
        OLEDUI_ShowDimming(nowMs);
        return;
    }

    if (!LED1_IsOn())
    {
        LED1_ON();
    }

    LED1_SetTarget(level);
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

        if (human)
        {
            s_app.humanTriggerCount++;
        }

        Protocol_SendHuman(human, s_app.humanTriggerCount);
    }

    App_ApplyAutoLight(nowMs);
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
            Protocol_SendGesture("NEAR");
            break;

        case GESTURE_EVENT_WAVE:
            if (s_app.mode == APP_MODE_MANUAL)
            {
                LED1_Turn();
                OLEDUI_ShowGesture(nowMs);
                Protocol_SendGesture("WAVE");
                Protocol_SendLight(LED1_IsOn());
            }
            else
            {
                OLEDUI_ShowGesture(nowMs);
                LED1_AckFlash(1);
                Protocol_SendGesture("WAVE_AUTO");
            }
            break;

        case GESTURE_EVENT_HOLD_START:
            s_app.gestureDimTick = nowMs;
            App_AdjustBrightness(0, nowMs);
            Protocol_SendDim("START");
            break;

        case GESTURE_EVENT_HOLDING:
            App_GestureDimming(nowMs);
            break;

        case GESTURE_EVENT_HOLD_END:
            s_app.gestureDimUp = !s_app.gestureDimUp;
            OLEDUI_ShowLightState(nowMs);
            Protocol_SendDim("END");
            break;

        default:
            break;
    }
}

static void App_HandleSerial(uint32_t nowMs)
{
    ProtocolPacket_t packet;

    if (!Protocol_FetchCommand(&packet))
    {
        return;
    }

    switch (packet.command)
    {
        case PROTOCOL_CMD_PING:
            Protocol_SendPong();
            break;

        case PROTOCOL_CMD_GET_STATUS:
            App_SendStatus();
            break;

        case PROTOCOL_CMD_MODE_AUTO:
            App_SetMode(APP_MODE_AUTO, nowMs);
            break;

        case PROTOCOL_CMD_MODE_MANUAL:
            App_SetMode(APP_MODE_MANUAL, nowMs);
            break;

        case PROTOCOL_CMD_MODE_TOGGLE:
            App_ToggleMode(nowMs);
            break;

        case PROTOCOL_CMD_LED_ON:
            if (s_app.mode != APP_MODE_AUTO || s_app.lastHuman)
            {
                LED1_ON();
            }
            OLEDUI_ShowLightState(nowMs);
            Protocol_SendOK("LED_ON");
            App_SendStatus();
            break;

        case PROTOCOL_CMD_LED_OFF:
            if (s_app.mode != APP_MODE_AUTO)
            {
                LED1_OFF();
            }
            OLEDUI_ShowLightState(nowMs);
            Protocol_SendOK("LED_OFF");
            App_SendStatus();
            break;

        case PROTOCOL_CMD_BRIGHT_UP:
            App_AdjustBrightness(10, nowMs);
            Protocol_SendOK("BRIGHT_UP");
            App_SendStatus();
            break;

        case PROTOCOL_CMD_BRIGHT_DOWN:
            App_AdjustBrightness(-10, nowMs);
            Protocol_SendOK("BRIGHT_DOWN");
            App_SendStatus();
            break;

        case PROTOCOL_CMD_BRIGHT_SET:
            App_SetBrightness((uint8_t)packet.value, nowMs);
            Protocol_SendOK("BRIGHT_SET");
            App_SendStatus();
            break;

        case PROTOCOL_CMD_COUNT_GET:
            Protocol_SendCount(s_app.humanTriggerCount);
            break;

        case PROTOCOL_CMD_COUNT_RESET:
            s_app.humanTriggerCount = 0;
            Protocol_SendCount(s_app.humanTriggerCount);
            break;

        default:
            Protocol_SendError("BAD_CMD");
            break;
    }
}

static void App_SyncStatus(void)
{
    uint8_t target;
    uint8_t light;

    if (!SystemTime_IsDue(&s_app.statusTick, STATUS_SYNC_MS))
    {
        return;
    }

    if (s_app.mode == APP_MODE_AUTO && !s_app.lastHuman)
    {
        target = Get_Brightness_Set();
    }
    else
    {
        target = LED1_GetTarget();
    }

    light = LED1_IsOn();

    OLEDUI_SetMode(s_app.mode == APP_MODE_AUTO);
    OLEDUI_SetHuman(s_app.lastHuman);
    OLEDUI_SetLight(light, LED1_GetBrightness());
    OLEDUI_SetBrightness(LED1_GetBrightness(), target);

    if (light != s_app.lastLight)
    {
        s_app.lastLight = light;
        Protocol_SendLight(light);
    }

    if (target != s_app.lastTarget)
    {
        s_app.lastTarget = target;
        Protocol_SendBrightness(LED1_GetBrightness(), target);
    }
}

void App_Init(void)
{
    s_app.mode = APP_MODE_MANUAL;
    s_app.lastHuman = LD2410B_GetState();
    s_app.lastLight = 0xFF;
    s_app.lastTarget = 0xFF;
    s_app.gestureDimUp = 1;
    s_app.humanTriggerCount = 0;

    s_app.radarTick = 0;
    s_app.gestureTick = 0;
    s_app.gestureDimTick = 0;
    s_app.statusTick = 0;

    Gesture_Init();
    OLEDUI_SetMode(0);
    OLEDUI_SetHuman(s_app.lastHuman);
    OLEDUI_ShowMode(0, SystemTime_Millis());

    Protocol_SendMode(0);
    App_SendStatus();
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
    OLEDUI_Task(now);
}
