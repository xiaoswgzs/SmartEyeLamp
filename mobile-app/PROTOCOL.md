# NightLight BLE UART Protocol v1

适用项目：智能护眼台灯 / 智能小夜灯  
通信方式：BLE 串口透传 / USART1  
串口参数：9600, 8N1  
编码建议：命令和状态字段使用 ASCII  
换行：每个数据包必须以 `\r\n` 结尾

## 基本帧格式

App 到台灯：

```text
@COMMAND\r\n
```

台灯到 App：

```text
#TYPE,K=V,K=V\r\n
```

示例：

```text
@GET\r\n
@MODE=AUTO\r\n
@BRIGHT=65\r\n
#STATUS,MODE=AUTO,LIGHT=1,BRIGHT=63,TARGET=65,HUMAN=1,HIT=12\r\n
```

## App 命令

| 命令 | 功能 | 典型返回 |
| --- | --- | --- |
| `@PING\r\n` | 通信测试 | `#PONG\r\n` |
| `@GET\r\n` | 获取完整状态 | `#STATUS,...\r\n` |
| `@STATUS?\r\n` | 获取完整状态，等价于 `GET` | `#STATUS,...\r\n` |
| `@MODE=AUTO\r\n` | 切换自动模式 | `#EVT,MODE=AUTO\r\n` + `#STATUS,...\r\n` |
| `@MODE=MANUAL\r\n` | 切换手动模式 | `#EVT,MODE=MANUAL\r\n` + `#STATUS,...\r\n` |
| `@MODE=TOGGLE\r\n` | 手动/自动模式切换 | `#EVT,MODE=...\r\n` + `#STATUS,...\r\n` |
| `@BRIGHT+\r\n` | 增加亮度 | `#OK,CMD=BRIGHT_UP\r\n` + `#STATUS,...\r\n` |
| `@BRIGHT-\r\n` | 降低亮度 | `#OK,CMD=BRIGHT_DOWN\r\n` + `#STATUS,...\r\n` |
| `@BRIGHT=65\r\n` | 设置亮度，范围 0-100 | `#OK,CMD=BRIGHT_SET\r\n` + `#STATUS,...\r\n` |
| `@COUNT?\r\n` | 查询人体触发次数 | `#COUNT,HUMAN_HIT=n\r\n` |
| `@COUNT_RESET\r\n` | 清零人体触发次数 | `#COUNT,HUMAN_HIT=0\r\n` |
| `@LED=ON\r\n` | 兼容开灯命令 | `#OK,CMD=LED_ON\r\n` + `#STATUS,...\r\n` |
| `@LED=OFF\r\n` | 兼容关灯命令 | `#OK,CMD=LED_OFF\r\n` + `#STATUS,...\r\n` |

## 兼容旧命令

| 旧命令 | 等价新命令 |
| --- | --- |
| `@MODE_AUTO\r\n` | `@MODE=AUTO\r\n` |
| `@MODE_MANUAL\r\n` | `@MODE=MANUAL\r\n` |
| `@MODE_TOGGLE\r\n` | `@MODE=TOGGLE\r\n` |
| `@LED_ON\r\n` | `@LED=ON\r\n` |
| `@LED_OFF\r\n` | `@LED=OFF\r\n` |
| `@BRTI\r\n` | `@BRIGHT+\r\n` |
| `@BRTD\r\n` | `@BRIGHT-\r\n` |

## 状态帧

```text
#STATUS,MODE=AUTO,LIGHT=1,BRIGHT=63,TARGET=65,HUMAN=1,HIT=12\r\n
```

| 字段 | 范围 | 含义 |
| --- | --- | --- |
| `MODE` | `AUTO` / `MANUAL` | 当前模式 |
| `LIGHT` | `0` / `1` | 当前灯光是否点亮 |
| `BRIGHT` | 0-100 | 当前实际亮度 |
| `TARGET` | 0-100 | 目标亮度 / 记忆亮度 |
| `HUMAN` | `0` / `1` | LD2410B 当前是否检测到人 |
| `HIT` | 0-N | 人体从无人到有人触发次数 |

## 主动事件

| 事件 | 含义 |
| --- | --- |
| `#EVT,MODE=AUTO\r\n` | 已进入自动模式 |
| `#EVT,MODE=MANUAL\r\n` | 已进入手动模式 |
| `#EVT,LIGHT=1\r\n` | 灯光状态变为开 |
| `#EVT,LIGHT=0\r\n` | 灯光状态变为关 |
| `#EVT,HUMAN=1,HIT=n\r\n` | 检测到人体，触发次数增加 |
| `#EVT,HUMAN=0,HIT=n\r\n` | 人离开，触发次数不增加 |
| `#EVT,BRIGHT=60,TARGET=65\r\n` | 亮度或目标亮度变化 |
| `#EVT,GESTURE=NEAR\r\n` | VL53L0X 检测到手掌靠近 |
| `#EVT,GESTURE=WAVE\r\n` | 手动模式下识别到挥手 |
| `#EVT,GESTURE=WAVE_AUTO\r\n` | 自动模式下识别到挥手，仅提示 |
| `#EVT,DIM=START\r\n` | 手势悬停调光开始 |
| `#EVT,DIM=END\r\n` | 手势悬停调光结束 |

## App 解析建议

1. BLE notify 接收后按 `\r\n` 切包。
2. 只解析 `#` 开头的消息。
3. 去掉 `#` 后按英文逗号分割。
4. 第一个字段为消息类型：`STATUS`、`EVT`、`OK`、`ERR`、`COUNT`、`PONG`。
5. 其余字段按 `K=V` 解析。
6. 发送命令必须带 `@` 包头和 `\r\n` 包尾。
7. BLE 单包建议不超过 20 字节，较长命令应按队列分包发送。
