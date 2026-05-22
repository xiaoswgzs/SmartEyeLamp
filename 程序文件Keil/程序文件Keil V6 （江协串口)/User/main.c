#include "stm32f10x.h"
#include "OLED.H"
#include "Serial.h"
#include "LED.h"
#include "Key.h"
#include "LD2410B.h"
#include "My_VL53L0X.h"
#include "OLED_UI.h"
#include "Gesture_ToF.h"
#include <stdio.h>
#include <string.h>

#define TOUCH_SCAN_MS       5
#define TOUCH_DEBOUNCE_MS  18
#define DIM_STEP_MS        25

#define RADAR_SCAN_MS      80
#define GESTURE_SCAN_MS    45
#define STATUS_SYNC_MS     80
#define PANEL_SYNC_MS     500

#define TOUCH_PORT         GPIOB
#define TOUCH_POWER_PIN    GPIO_Pin_1
#define TOUCH_UP_PIN       GPIO_Pin_11
#define TOUCH_DOWN_PIN     GPIO_Pin_0

typedef struct
{
    GPIO_TypeDef *Port;
    uint16_t Pin;

    uint8_t Raw;
    uint8_t Stable;
    uint8_t PressEvent;

    uint32_t ChangeTime;
} TouchKey_t;

static volatile uint32_t g_ms = 0;

static TouchKey_t TouchPower;
static TouchKey_t TouchUp;
static TouchKey_t TouchDown;

static uint32_t TouchTick = 0;
static uint32_t DimTick = 0;
static uint32_t RadarTick = 0;
static uint32_t GestureTick = 0;
static uint32_t GestureDimTick = 0;
static uint32_t StatusTick = 0;
static uint32_t PanelTick = 0;

static uint8_t LastHuman = 0;
static uint8_t GestureDimDirection = 1;
static uint8_t DebugMode = 0;
static uint8_t PanelLastBrightness = 0xFF;
static uint8_t PanelLastLedState = 0xFF;

static uint32_t Millis(void)
{
    return g_ms;
}

static uint8_t TimeDue(uint32_t *lastTime, uint16_t interval)
{
    uint32_t now;

    now = Millis();

    if ((uint32_t)(now - *lastTime) >= interval)
    {
        *lastTime = now;
        return 1;
    }

    return 0;
}

static void TimeBase_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseStructure.TIM_Period = 10 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM4, ENABLE);
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
    {
        g_ms++;
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}

static void TouchKey_InitState(TouchKey_t *key, GPIO_TypeDef *port, uint16_t pin)
{
    key->Port = port;
    key->Pin = pin;
    key->Raw = GPIO_ReadInputDataBit(port, pin);
    key->Stable = key->Raw;
    key->PressEvent = 0;
    key->ChangeTime = Millis();
}

static void Touch_InitState(void)
{
    TouchKey_InitState(&TouchPower, TOUCH_PORT, TOUCH_POWER_PIN);
    TouchKey_InitState(&TouchUp, TOUCH_PORT, TOUCH_UP_PIN);
    TouchKey_InitState(&TouchDown, TOUCH_PORT, TOUCH_DOWN_PIN);
}

static void TouchKey_Update(TouchKey_t *key)
{
    uint8_t raw;
    uint32_t now;

    raw = GPIO_ReadInputDataBit(key->Port, key->Pin);
    now = Millis();

    if (raw != key->Raw)
    {
        key->Raw = raw;
        key->ChangeTime = now;
    }

    if ((uint32_t)(now - key->ChangeTime) >= TOUCH_DEBOUNCE_MS)
    {
        if (key->Stable != key->Raw)
        {
            key->Stable = key->Raw;

            if (key->Stable == 1)
            {
                key->PressEvent = 1;
            }
        }
    }
}

static uint8_t TouchKey_TakePress(TouchKey_t *key)
{
    if (key->PressEvent)
    {
        key->PressEvent = 0;
        return 1;
    }

    return 0;
}

static void Touch_Task(void)
{
    uint8_t upPress;
    uint8_t downPress;
    uint32_t now;

    if (!TimeDue(&TouchTick, TOUCH_SCAN_MS))
    {
        return;
    }

    TouchKey_Update(&TouchPower);
    TouchKey_Update(&TouchUp);
    TouchKey_Update(&TouchDown);

    now = Millis();

    if (TouchKey_TakePress(&TouchPower))
    {
        LED1_Turn();
        OLEDUI_ShowLightState(now);
    }

    upPress = TouchKey_TakePress(&TouchUp);
    downPress = TouchKey_TakePress(&TouchDown);

    if (upPress && TouchDown.Stable == 0)
    {
        LED1_Inc();
        DimTick = now;
        OLEDUI_ShowDimming(now);
    }
    else if (downPress && TouchUp.Stable == 0)
    {
        LED1_Dec();
        DimTick = now;
        OLEDUI_ShowDimming(now);
    }

    if (TouchUp.Stable && TouchDown.Stable == 0)
    {
        if ((uint32_t)(now - DimTick) >= DIM_STEP_MS)
        {
            LED1_Inc();
            DimTick = now;
            OLEDUI_ShowDimming(now);
        }
    }
    else if (TouchDown.Stable && TouchUp.Stable == 0)
    {
        if ((uint32_t)(now - DimTick) >= DIM_STEP_MS)
        {
            LED1_Dec();
            DimTick = now;
            OLEDUI_ShowDimming(now);
        }
    }
    else
    {
        DimTick = now;
    }
}

static void Radar_Task(void)
{
    uint8_t human;

    if (!TimeDue(&RadarTick, RADAR_SCAN_MS))
    {
        return;
    }

    human = LD2410B_GetState();

    if (human == LastHuman)
    {
        return;
    }

    LastHuman = human;

    if (human)
    {
        LED1_ON();
        printf("[d,1,21,人体存在：是]");
    }
    else
    {
        LED1_OFF();
        printf("[d,1,21,人体存在：否]");
    }

    OLEDUI_ShowLightState(Millis());
}

static void Gesture_DimmingStep(uint32_t now)
{
    uint8_t target;

    if ((uint32_t)(now - GestureDimTick) < 55)
    {
        return;
    }

    GestureDimTick = now;
    target = LED1_GetTarget();

    if (GestureDimDirection)
    {
        if (target >= 100)
        {
            GestureDimDirection = 0;
            LED1_AddTarget(-1);
        }
        else
        {
            LED1_AddTarget(1);
        }
    }
    else
    {
        if (target <= 12)
        {
            GestureDimDirection = 1;
            LED1_AddTarget(1);
        }
        else
        {
            LED1_AddTarget(-1);
        }
    }

    OLEDUI_ShowDimming(now);
}

static void Gesture_Task(void)
{
    uint16_t distance;
    uint32_t now;
    GestureEvent_t event;

    if (!TimeDue(&GestureTick, GESTURE_SCAN_MS))
    {
        return;
    }

    now = Millis();
    distance = MyVL53L0X_GetDistance();
    OLEDUI_SetDistance(distance);

    event = Gesture_Update(distance, now);

    switch (event)
    {
        case GESTURE_EVENT_NEAR:
            OLEDUI_ShowGesture(now);
            break;

        case GESTURE_EVENT_WAVE:
            LED1_Turn();
            OLEDUI_ShowGesture(now);
            Serial_SendString("GESTURE_WAVE\r\n");
            break;

        case GESTURE_EVENT_DOUBLE_WAVE:
            DebugMode = !DebugMode;
            OLEDUI_SetDebug(DebugMode);
            Serial_SendString("GESTURE_MODE\r\n");
            break;

        case GESTURE_EVENT_HOLD_START:
            if (LED1_GetTarget() == 0)
            {
                LED1_ON();
            }
            GestureDimTick = now;
            OLEDUI_ShowDimming(now);
            Serial_SendString("GESTURE_DIM_START\r\n");
            break;

        case GESTURE_EVENT_HOLDING:
            Gesture_DimmingStep(now);
            break;

        case GESTURE_EVENT_HOLD_END:
            GestureDimDirection = !GestureDimDirection;
            OLEDUI_ShowLightState(now);
            Serial_SendString("GESTURE_DIM_END\r\n");
            break;

        default:
            break;
    }
}

static void Status_Task(void)
{
    if (!TimeDue(&StatusTick, STATUS_SYNC_MS))
    {
        return;
    }

    OLEDUI_SetLight(LED1_IsOn(), LED1_GetBrightness());
    OLEDUI_SetBrightness(LED1_GetBrightness(), LED1_GetTarget());
}

static void Panel_Task(void)
{
    uint8_t brightness;
    uint8_t ledState;

    if (!TimeDue(&PanelTick, PANEL_SYNC_MS))
    {
        return;
    }

    brightness = LED1_GetTarget();
    ledState = LED1_IsOn();

    if (brightness != PanelLastBrightness)
    {
        PanelLastBrightness = brightness;
        printf("[d,1,61,当前亮度为：%03d]", brightness);
    }

    if (ledState != PanelLastLedState)
    {
        PanelLastLedState = ledState;
        if (ledState)
        {
            printf("[d,1,21,灯光状态：开]");
        }
        else
        {
            printf("[d,1,21,灯光状态：关]");
        }
    }
}

static void Serial_Task(void)
{
    if (Serial_RXFlag == 0)
    {
        return;
    }

    if (strcmp(Serial_RXPacket, "LED_ON") == 0)
    {
        LED1_ON();
        OLEDUI_ShowLightState(Millis());
        Serial_SendString("LED_ON_OK\r\n");
    }
    else if (strcmp(Serial_RXPacket, "LED_OFF") == 0)
    {
        LED1_OFF();
        OLEDUI_ShowLightState(Millis());
        Serial_SendString("LED_OFF_OK\r\n");
    }
    else if (strcmp(Serial_RXPacket, "BRTI") == 0)
    {
        LED1_AddTarget(10);
        OLEDUI_ShowDimming(Millis());
        Serial_SendString("BRTI_OK\r\n");
    }
    else if (strcmp(Serial_RXPacket, "BRTD") == 0)
    {
        LED1_AddTarget(-10);
        OLEDUI_ShowDimming(Millis());
        Serial_SendString("BRTD_OK\r\n");
    }
    else if (strcmp(Serial_RXPacket, "DEBUG") == 0)
    {
        DebugMode = !DebugMode;
        OLEDUI_SetDebug(DebugMode);
        Serial_SendString("DEBUG_OK\r\n");
    }
    else
    {
        Serial_SendString("Command_Error\r\n");
    }

    Serial_RXFlag = 0;
}

static void Board_Init(void)
{
    TimeBase_Init();

    OLED_Init();
    OLEDUI_Init();

    Key_Init();
    Serial_Init();
    LED_Init();
    LD2410B_Init();
    MyVL53L0X_Init();
    Gesture_Init();

    Touch_InitState();

    printf("[d,1,81,极简护眼小夜灯]");
    printf("[d,1,111,      CodeCat Studio]");
}

int main(void)
{
    Board_Init();

    while (1)
    {
        Touch_Task();
        Gesture_Task();
        Serial_Task();
        Radar_Task();

        LED1_Task(Millis());
        Status_Task();
        Panel_Task();
        OLEDUI_Task(Millis());
    }
}
