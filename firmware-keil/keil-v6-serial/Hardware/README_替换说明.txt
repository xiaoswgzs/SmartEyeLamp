智能小夜灯 V3 重构版

主要变化：
1. main.c 只保留 Board_Init() 和 App_Task()，主循环不再堆业务逻辑。
2. PB1 不再控制灯光开关，改为手动/自动模式切换。
3. PB11/PB0 负责亮度加减，长按会连续快速调光。
4. 手动模式：VL53L0X 挥手开关灯，悬停调光。
5. 自动模式：LD2410B 人来亮灯，人走灭灯；挥手可临时关闭，等人离开后自动恢复。
6. 模式切换成功后，灯光会做非阻塞轻闪确认，不影响主循环。
7. OLED 刷新做了限速：动画帧 80ms，I2C 每 4ms 只刷一页，降低卡顿。

Keil 操作：
- 替换：main.c、LED.c/h、PWM.c/h、OLED_UI.c/h、OLED_GFX.c/h、Gesture_ToF.c/h、OLED_Font.c/h
- 新增并加入工程编译：SystemTime.c、Touch.c、App.c、OLED_Font.c、Gesture_ToF.c、OLED_GFX.c、OLED_UI.c
- 如果你工程里已经有 TIM4_IRQHandler，把 SystemTime.c 里的 TIM4_IRQHandler 合并过去，或把 SystemTime 改用 TIM3。
