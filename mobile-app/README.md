# CodeCat NightLight App

基于 uni-app + Vue 3 的智能护眼台灯 BLE 控制 App，用于连接台灯蓝牙串口透传模块，并通过 `@COMMAND\r\n` 协议控制固件。

## 功能

- BLE 设备扫描、连接、断开
- 自动发现常见 BLE UART 服务与特征值
- 查询台灯状态
- 手动/自动模式切换
- 亮度滑杆调节与快捷增减
- 人体存在状态显示
- 人体触发次数查询与清零
- BLE 通信日志折叠面板

## 目录

```text
mobile-app/
+-- App.vue
+-- main.js
+-- manifest.json
+-- pages.json
+-- package.json
+-- PROTOCOL.md
+-- pages/index/index.vue
+-- utils/ble.js
+-- utils/protocol.js
```

`unpackage/` 是 HBuilderX/uni-app 构建输出，已在仓库根目录 `.gitignore` 中排除。

## 运行

1. 使用 HBuilderX 打开 `mobile-app/`。
2. 运行到 Android 真机。
3. 授予蓝牙和定位权限。
4. 在 App 内扫描并连接台灯 BLE 串口透传模块。
5. 使用模式切换、亮度调节、人体触发统计等功能。

也可以使用命令行构建，但需要先安装 uni-app 对应 CLI：

```bash
npm install
npm run dev:app
```

## 通信

App 向台灯发送：

```text
@COMMAND\r\n
```

台灯返回：

```text
#TYPE,K=V,K=V\r\n
```

完整协议见 [PROTOCOL.md](./PROTOCOL.md)。
