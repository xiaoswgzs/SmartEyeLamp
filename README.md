# Smart Eye Lamp

智能护眼台灯 / 智能小夜灯原型项目。仓库包含 STM32F103C8 固件、uni-app + Vue 3 移动端 App、Proteus 核心电路仿真、PCB/3D 外壳模型和器材报价表。项目主要实现手动调光、人体存在检测自动亮灭、ToF 手势控制、OLED 状态显示以及串口/BLE 透传控制。

## 项目概览

- 主控：STM32F103C8，基于 STM32F10x 标准外设库开发。
- 固件工程：Keil uVision / MDK-ARM 工程。
- 人体存在检测：LD2410B 雷达模块，检测到有人时自动开灯，无人时自动关灯。
- 手势/距离检测：VL53L0X ToF 模块，使用官方 VL53L0X API 并移植到标准库工程。
- 交互：触摸按键、OLED 显示、USART1 串口协议，可接 BLE 串口透传模块。
- 输出：TIM2_CH3 PWM 控制灯光亮度，带亮度记忆、平滑渐变和操作反馈闪烁。
- 移动端：uni-app + Vue 3 App，通过 BLE UART 与固件串口协议通信。

## 目录结构

```text
.
+-- firmware-keil/          # Keil 固件工程，包含 V1-V7 历史版本
|   +-- keil-v1/
|   +-- keil-v2/
|   +-- keil-v3/
|   +-- keil-v4-serial/
|   +-- keil-v5-serial/
|   +-- keil-v6-serial/
|   +-- keil-v7-final/      # 推荐使用的最终版本
+-- mobile-app/             # uni-app + Vue 3 BLE 控制 App 源码
+-- pcb-3d-models/          # PCB/底板/外壳 3D 模型文件
+-- simulation/             # Proteus 仿真工程
|   +-- project-backups/    # Proteus 自动备份
+-- .gitignore
+-- README.md
+-- 器材报价表.xlsx
```

本次已规范化的目录命名：

| 原目录 | 新目录 |
| --- | --- |
| `PCB&3D` | `pcb-3d-models` |
| `仿真` | `simulation` |
| `仿真/Project Backups` | `simulation/project-backups` |
| `程序文件Keil` | `firmware-keil` |
| `程序文件Keil V1` - `V3` | `keil-v1` - `keil-v3` |
| `程序文件Keil V4 （江协串口）` | `keil-v4-serial` |
| `程序文件Keil V5 （江协串口)` | `keil-v5-serial` |
| `程序文件Keil V6 （江协串口)` | `keil-v6-serial` |
| `程序文件Keil V7 （江协串口)最终版本` | `keil-v7-final` |
| `nightlight_uniapp_vue3_apple_clean_v2/nightlight_uniapp_vue3` | `mobile-app` |

## 推荐工程入口

推荐打开：

```text
firmware-keil/keil-v7-final/Project.uvprojx
```

V7 是当前最完整版本，相比 V6 增加了 `Protocol.c/.h`，串口控制协议更清晰，支持状态查询、亮度设置、人体触发计数等功能。

## App 工程

移动端源码位于：

```text
mobile-app/
```

这是一个 uni-app + Vue 3 App，面向 Android 真机运行，通过 BLE 串口透传模块连接台灯。核心文件：

| 文件 | 作用 |
| --- | --- |
| `pages/index/index.vue` | 主界面，包含连接、模式切换、亮度控制、人体触发统计和通信日志 |
| `utils/ble.js` | BLE 扫描、连接、UART 服务发现、notify 接收和分包发送 |
| `utils/protocol.js` | `@COMMAND\r\n` 命令生成、`#TYPE,K=V` 回包解析和状态合并 |
| `manifest.json` | uni-app 应用信息和 Android 蓝牙权限配置 |
| `PROTOCOL.md` | App 与固件之间的 BLE UART 协议文档 |

App 运行步骤：

1. 使用 HBuilderX 打开 `mobile-app/`。
2. 运行到 Android 真机。
3. 授予蓝牙和定位权限。
4. 在 App 中扫描并连接台灯 BLE 串口透传模块。
5. 使用模式切换、亮度调节、状态查询和人体触发统计功能。

`mobile-app/unpackage/` 是 HBuilderX/uni-app 构建输出，已通过 `.gitignore` 排除，不作为源码提交。

## 固件模块说明

`firmware-keil/keil-v7-final/` 的主要目录：

| 目录 | 说明 |
| --- | --- |
| `User/` | 用户入口，`main.c` 初始化板级外设并循环执行 `App_Task()` |
| `Hardware/` | 业务和硬件驱动模块 |
| `System/` | 延时等基础系统模块 |
| `Start/` | CMSIS 启动文件、系统初始化文件和 STM32 头文件 |
| `Library/` | STM32F10x 标准外设库 |
| `VL53L0X_API/` | ST VL53L0X 官方 API 及平台移植层 |
| `Objects/` | Keil 编译输出 |
| `Listings/` | Keil map/listing 输出 |

核心业务模块：

| 文件 | 作用 |
| --- | --- |
| `Hardware/App.c` | 主业务状态机，整合触摸、雷达、ToF 手势、串口协议、OLED 和 LED |
| `Hardware/Protocol.c` | 串口/BLE 透传控制协议解析和事件回传 |
| `Hardware/LED.c` | 灯光亮度、记忆亮度、平滑渐变、反馈闪烁 |
| `Hardware/PWM.c` | TIM2_CH3 PWM 输出 |
| `Hardware/LD2410B.c` | LD2410B 人体存在检测输入 |
| `Hardware/My_VL53L0X.c` | VL53L0X 初始化、校准和连续测距 |
| `Hardware/Gesture_ToF.c` | 基于 ToF 距离的靠近、挥手、长按调光识别 |
| `Hardware/Touch.c` | 触摸按键扫描、消抖、长按重复调光 |
| `Hardware/OLED_UI.c` | OLED 状态界面和提示场景 |
| `Hardware/SystemTime.c` | TIM4 1 ms 系统时基 |
| `Hardware/Serial.c` | USART1 收发、`printf` 重定向、`@...\r\n` 包接收 |

## 硬件连接

从代码中整理出的主要引脚如下：

| 功能 | MCU 引脚 | 说明 |
| --- | --- | --- |
| 灯光 PWM | PA2 / TIM2_CH3 | 输出 PWM 控制灯光亮度 |
| USART1 TX | PA9 | 串口发送，可连接 BLE 串口模块 RX |
| USART1 RX | PA10 | 串口接收，可连接 BLE 串口模块 TX |
| OLED SCL | PB8 | 软件 I2C |
| OLED SDA | PB9 | 软件 I2C |
| VL53L0X SCL | PB12 | 软件 I2C |
| VL53L0X SDA | PB13 | 软件 I2C |
| LD2410B OUT | PB10 | 人体存在检测输入，下拉输入 |
| 模式触摸键 | PB1 | 手动/自动模式切换 |
| 亮度增加键 | PB11 | 手动调亮，支持按住连续调整 |
| 亮度降低键 | PB0 | 手动调暗，支持按住连续调整 |

## 功能说明

### 工作模式

- 手动模式：触摸键或串口控制灯光开关、亮度调整；ToF 挥手可切换灯光。
- 自动模式：根据 LD2410B 的人体存在状态控制灯光。检测到有人开灯，检测不到人关灯。
- 自动模式下无人时调整亮度，会修改记忆亮度；下次有人触发时按记忆亮度点亮。

### 触摸控制

- PB1：切换手动/自动模式。
- PB11：增加亮度。
- PB0：降低亮度。
- 亮度键支持按住连续调节。

### ToF 手势控制

`Gesture_ToF.c` 基于 VL53L0X 距离数据识别：

- 靠近：距离进入近距离阈值后触发提示。
- 挥手：短时间靠近后离开，在手动模式下切换灯光开关。
- 长按：手停留超过设定时间后进入连续调光；松手结束调光并反转下一次调光方向。

### OLED 显示

OLED 用于显示当前模式、人体检测状态、灯光状态、亮度和手势/调光提示。V6/V7 中也保留了部分串口屏格式输出字符串，当前源码注释和部分中文字符串存在编码乱码，建议后续统一转为 UTF-8 或改为纯英文调试输出。

## 串口协议

USART1 参数：

```text
9600 baud, 8 data bits, 1 stop bit, no parity
```

接收格式：

```text
@COMMAND\r\n
```

发送格式：

```text
#TYPE,K=V\r\n
```

V7 支持的常用命令：

| 命令 | 说明 |
| --- | --- |
| `@PING\r\n` | 设备回复 `#PONG` |
| `@GET\r\n` 或 `@STATUS?\r\n` | 查询当前状态 |
| `@MODE=AUTO\r\n` 或 `@MODE_AUTO\r\n` | 切换自动模式 |
| `@MODE=MANUAL\r\n` 或 `@MODE_MANUAL\r\n` | 切换手动模式 |
| `@MODE=TOGGLE\r\n` 或 `@MODE_TOGGLE\r\n` | 切换模式 |
| `@LED=ON\r\n` 或 `@LED_ON\r\n` | 开灯 |
| `@LED=OFF\r\n` 或 `@LED_OFF\r\n` | 关灯 |
| `@BRIGHT+\r\n`、`@BRIGHT_UP\r\n`、`@BRTI\r\n` | 增加亮度 |
| `@BRIGHT-\r\n`、`@BRIGHT_DOWN\r\n`、`@BRTD\r\n` | 降低亮度 |
| `@BRIGHT=60\r\n` | 设置亮度百分比，范围 0-100 |
| `@COUNT?\r\n` 或 `@HUMAN_COUNT?\r\n` | 查询人体触发次数 |
| `@COUNT_RESET\r\n` 或 `@HUMAN_COUNT_RESET\r\n` | 清零人体触发次数 |

App 端使用同一套命令，协议细节也同步记录在 `mobile-app/PROTOCOL.md`。

典型回包：

```text
#STATUS,MODE=MANUAL,LIGHT=1,BRIGHT=65,TARGET=65,HUMAN=0,HIT=3
#EVT,MODE=AUTO
#EVT,HUMAN=1,HIT=4
#EVT,BRIGHT=52,TARGET=60
#ERR,CODE=BAD_CMD
```

## 构建与烧录

1. 安装 Keil MDK-ARM，并确保已安装 STM32F1 相关 Device Pack。
2. 打开 `firmware-keil/keil-v7-final/Project.uvprojx`。
3. 检查目标芯片为 `STM32F103C8`。
4. 编译工程，输出文件位于 `firmware-keil/keil-v7-final/Objects/`。
5. 使用 ST-Link 或其他下载器烧录到 STM32F103C8。
6. 若使用串口/BLE 控制，连接 USART1：PA9 为 TX，PA10 为 RX，波特率 9600。

## 仿真与模型

- Proteus 仿真工程：`simulation/核心电路仿真.pdsprj`
- 仿真备份：`simulation/project-backups/`
- 3D/外壳模型：`pcb-3d-models/`
  - `智能小夜灯(无底板).stl`
  - `智能小夜灯2(有底板).stl`
  - `智能小夜灯2(有底板).3mf`
  - `智能小夜灯底板.stl`
- 器材报价表：`器材报价表.xlsx`

## 版本演进

| 版本目录 | 说明 |
| --- | --- |
| `keil-v1` | 早期基础版本 |
| `keil-v2` | 早期迭代版本 |
| `keil-v3` | 加入/整理 VL53L0X API、OLED、雷达等模块 |
| `keil-v4-serial` | 加入串口相关能力 |
| `keil-v5-serial` | 串口版本继续迭代 |
| `keil-v6-serial` | 应用层状态机、触摸、手势、OLED UI 较完整 |
| `keil-v7-final` | 推荐版本，增加协议层和更完整的状态/事件回传 |

## 已知注意事项

- 仓库历史中已经包含 Keil 编译产物，如 `Objects/`、`Listings/`、`.uvguix.*` 和部分 `.vscode/keil-assistant.log`。本轮已新增 `.gitignore` 排除 App 构建产物，后续可继续清理固件构建输出。
- 固件源码里仍有一些中文注释和显示字符串呈现乱码，通常是 GBK/UTF-8 编码混用导致。建议后续统一编码，或将串口调试/OLED 文本改成 ASCII。
- `Serial_RXPacket` 长度为 100 字节，串口接收状态机当前对超长输入的边界保护较弱，实际使用时应避免发送超过缓冲区的命令。
- `VL53L0X` 和 `OLED` 使用不同的软件 I2C 引脚；硬件连线时不要混接。

## 后续整理建议

- 扩展 `.gitignore`，继续过滤 Keil 构建输出和用户本地配置。
- 将最终版本单独整理为主固件目录，历史版本可归档到 `archive/`。
- 修复源码注释/字符串编码，避免后续维护时继续扩散乱码。
- 给 BLE App 增加真机截图、常见 BLE 模块 UUID 配置说明和发布包构建说明。
