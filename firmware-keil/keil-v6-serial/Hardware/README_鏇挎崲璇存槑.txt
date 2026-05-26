智能小夜灯 V4 Clean Refactor

这版按新的产品逻辑重构：
1. PB1 不再控制开关灯，只负责手动/自动模式切换。
2. 手动模式：VL53L0X 快速挥手开关灯，悬停调光。
3. 自动模式：LD2410B 雷达优先，有人亮、无人灭。
4. 自动模式下手势仍可悬停调光，但不会破坏“有人亮、无人灭”的规则。
5. 已删除 Debug 显示入口，OLED 只做产品状态动画。
6. LED 渐变速度加快，模式切换会有非阻塞的灵动闪烁反馈。

Keil 需要加入/替换这些文件：
- main.c
- App.c / App.h
- Touch.c / Touch.h
- SystemTime.c / SystemTime.h
- LED.c / LED.h
- Gesture_ToF.c / Gesture_ToF.h
- OLED_UI.c / OLED_UI.h
- OLED_GFX.c / OLED_GFX.h
- OLED_Font.c / OLED_Font.h
- PWM.c / PWM.h
- Key.c

注意：
- 如果工程里已经有 TIM4_IRQHandler，需要和 SystemTime.c 合并，或者把 SystemTime 改成 TIM3。
- OLED_Font.h 只保留 extern 声明，字体表只在 OLED_Font.c 定义一次。
- 不要再把旧 main.c 里的 Touch_Task / Radar_Task / Gesture_Task 留在工程里，否则会重复逻辑。
