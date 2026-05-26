NightLight V3 update

Main changes:
1. PB1 no longer toggles lamp power. Short press toggles AUTO/MANUAL mode. Long press toggles OLED debug page.
2. PB11/PB0 remain brightness up/down. Hold response is faster.
3. VL53L0X gesture controls lamp power and hand-hold dimming.
4. LD2410B presence control only works in AUTO mode: human -> ON, no human -> OFF.
5. Mode switch triggers non-blocking LED acknowledgement blink.
6. LED fade is faster, with a soft-linear PWM curve.
7. OLED animation refreshes two pages per task call for smoother display.

Keil notes:
- Add Gesture_ToF.c if not already added.
- Add OLED_Font.c if your font table was moved out of OLED_Font.h.
- Replace OLED_GFX.c/h, OLED_UI.c/h, LED.c/h, main.c.
- PWM.c/h are included because PWM_SetCompare3 uses uint16_t and TOP=1000.
