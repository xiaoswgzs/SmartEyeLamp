<template>
  <view class="page">
    <view class="topbar">
      <view>
        <text class="brand">CodeCat Light</text>
        <text class="headline">让光安静地陪着你</text>
      </view>

      <view :class="['connect-dot', connected ? 'online' : 'offline']" @tap="toggleConnectionPanel">
        <view class="dot"></view>
        <text>{{ connected ? '已连接' : '连接' }}</text>
      </view>
    </view>

    <view class="lamp-stage">
      <view :class="['glow-ring', status.light ? 'active' : 'sleep']"></view>
      <view :class="['lamp-orb', status.light ? 'orb-active' : 'orb-sleep']">
        <view class="orb-inner"></view>
      </view>
      <text class="mode-title">{{ status.light ? 'Light On' : 'Standby' }}</text>
      <text class="mode-subtitle">{{ connected ? stateLine : '连接台灯后开始控制' }}</text>
    </view>

    <view class="glass-card main-control">
      <view class="segmented">
        <view :class="['seg-item', status.mode === 'MANUAL' ? 'selected' : '']" @tap="sendModeManual">手动</view>
        <view :class="['seg-item', status.mode === 'AUTO' ? 'selected' : '']" @tap="sendModeAuto">自动</view>
      </view>

      <view class="brightness-head">
        <view>
          <text class="small-label">Brightness</text>
          <text class="brightness-value">{{ localBrightness }}%</text>
        </view>
        <button class="tiny-btn" @tap="refreshStatus">刷新</button>
      </view>

      <slider
        class="slider"
        :value="localBrightness"
        min="0"
        max="100"
        block-size="24"
        activeColor="#f5d06f"
        backgroundColor="rgba(255,255,255,0.16)"
        @changing="onSliderChanging"
        @change="onSliderChange"
      />

      <view class="quick-row">
        <button class="round-btn" @tap="sendBrightDown">-</button>
        <button class="mode-toggle-btn" @tap="sendModeToggle">切换模式</button>
        <button class="round-btn" @tap="sendBrightUp">+</button>
      </view>
    </view>

    <view class="compact-grid">
      <view class="mini-card">
        <text class="mini-label">人体存在</text>
        <text class="mini-value">{{ status.human ? '有人' : '无人' }}</text>
      </view>
      <view class="mini-card">
        <text class="mini-label">触发次数</text>
        <text class="mini-value">{{ status.hit }}</text>
      </view>
      <view class="mini-card">
        <text class="mini-label">目标亮度</text>
        <text class="mini-value">{{ status.target }}%</text>
      </view>
    </view>

    <view class="glass-card count-panel">
      <view>
        <text class="panel-title">人体触发统计</text>
        <text class="panel-desc">统计 LD2410B 从无人到有人触发的次数。</text>
      </view>
      <view class="count-actions">
        <button class="soft-chip" @tap="sendCountGet">查询</button>
        <button class="soft-chip danger" @tap="sendCountReset">清零</button>
      </view>
    </view>

    <view class="fold-card">
      <view class="fold-header" @tap="toggleConnectionPanel">
        <view>
          <text class="fold-title">蓝牙连接</text>
          <text class="fold-subtitle">{{ connectionText }}</text>
        </view>
        <text class="chevron">{{ connectionOpen ? '收起' : '展开' }}</text>
      </view>

      <view v-if="connectionOpen" class="fold-body">
        <view class="connection-actions">
          <button class="primary-action" @tap="scanDevices">{{ scanning ? '扫描中...' : '扫描设备' }}</button>
          <button v-if="connected" class="secondary-action" @tap="disconnect">断开</button>
        </view>

        <view v-if="devices.length === 0" class="empty">
          <text>未发现设备。请确认台灯蓝牙模块已上电，并授予蓝牙和定位权限。</text>
        </view>

        <view v-for="device in devices" :key="device.deviceId" class="device" @tap="connectDevice(device)">
          <view>
            <text class="device-name">{{ device.name }}</text>
            <text class="device-id">{{ shortDeviceId(device.deviceId) }}</text>
          </view>
          <text class="rssi">{{ device.RSSI }} dBm</text>
        </view>
      </view>
    </view>

    <view class="fold-card">
      <view class="fold-header" @tap="logOpen = !logOpen">
        <view>
          <text class="fold-title">通信日志</text>
          <text class="fold-subtitle">调试 BLE 协议时再打开</text>
        </view>
        <text class="chevron">{{ logOpen ? '收起' : '展开' }}</text>
      </view>

      <view v-if="logOpen" class="fold-body">
        <button class="clear-btn" @tap="clearLog">清空日志</button>
        <scroll-view class="log" scroll-y>
          <view v-for="(item, index) in logs" :key="index" class="log-line">
            <text>{{ item }}</text>
          </view>
        </scroll-view>
      </view>
    </view>
  </view>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from 'vue'
import { BleUartClient } from '../../utils/ble.js'
import { CMD, parseFrame, applyFrameToStatus } from '../../utils/protocol.js'

const connected = ref(false)
const scanning = ref(false)
const connectionOpen = ref(false)
const logOpen = ref(false)
const devices = ref([])
const logs = ref([])
const localBrightness = ref(60)

const status = reactive({
  mode: 'MANUAL',
  light: false,
  bright: 0,
  target: 60,
  human: false,
  hit: 0
})

let ble = null

const stateLine = computed(() => {
  if (status.mode === 'AUTO') {
    return status.human ? '自动模式 | 检测到有人' : '自动模式 | 等待靠近'
  }
  return status.light ? '手动模式 | 手势控制中' : '手动模式 | 轻挥唤醒'
})

const connectionText = computed(() => {
  if (connected.value) return '台灯已连接，可以正常控制'
  if (scanning.value) return '正在搜索附近的 BLE 设备'
  return '折叠收纳，保持主界面干净'
})

onMounted(() => {
  ble = new BleUartClient({
    onDeviceFound(list) {
      devices.value = list
        .filter((item) => item.name && item.name !== 'Unknown BLE')
        .sort((a, b) => (b.RSSI || -100) - (a.RSSI || -100))
    },

    onFrame(line) {
      addLog(`收到：${line}`)
      const frame = parseFrame(line)
      if (!frame) return

      const next = applyFrameToStatus(status, frame)
      Object.assign(status, next)

      if (frame.type === 'STATUS' || frame.type === 'EVT') {
        localBrightness.value = Number(status.target || status.bright || 0)
      }
    },

    onRaw() {},

    onLog(message) {
      addLog(message)
    },

    onState(state) {
      connected.value = !!state.connected
      if (connected.value) {
        connectionOpen.value = false
      }
    }
  })
})

function toggleConnectionPanel() {
  connectionOpen.value = !connectionOpen.value
}

function addLog(message) {
  const time = formatTime(new Date())
  logs.value.unshift(`[${time}] ${message}`)
  if (logs.value.length > 80) {
    logs.value.pop()
  }
}

function clearLog() {
  logs.value = []
}

async function scanDevices() {
  try {
    connectionOpen.value = true
    scanning.value = true
    devices.value = []
    await ble.startScan()

    setTimeout(async () => {
      scanning.value = false
      try {
        await ble.stopScan()
      } catch (error) {}
    }, 8000)
  } catch (error) {
    scanning.value = false
    showToast('扫描失败，请检查蓝牙和定位权限')
    addLog(`扫描失败：${formatError(error)}`)
  }
}

async function connectDevice(device) {
  try {
    showLoading('连接中')
    await ble.connect(device.deviceId)
    hideLoading()
    showToast('连接成功')
    await send(CMD.get())
  } catch (error) {
    hideLoading()
    showToast('连接失败')
    addLog(`连接失败：${formatError(error)}`)
  }
}

async function disconnect() {
  await ble.disconnect()
}

function refreshStatus() {
  send(CMD.get())
}

function sendModeToggle() {
  send(CMD.modeToggle())
}

function sendModeAuto() {
  send(CMD.modeAuto())
}

function sendModeManual() {
  send(CMD.modeManual())
}

function sendBrightUp() {
  localBrightness.value = Math.min(100, Number(localBrightness.value) + 5)
  send(CMD.brightUp())
}

function sendBrightDown() {
  localBrightness.value = Math.max(0, Number(localBrightness.value) - 5)
  send(CMD.brightDown())
}

function sendBrightSet() {
  send(CMD.brightSet(localBrightness.value))
}

function onSliderChanging(event) {
  localBrightness.value = event.detail.value
}

function onSliderChange(event) {
  localBrightness.value = event.detail.value
  sendBrightSet()
}

function sendCountGet() {
  send(CMD.countGet())
}

function sendCountReset() {
  uni.showModal({
    title: '确认清零',
    content: '确定要清零人体触发次数吗？',
    success(res) {
      if (res.confirm) {
        send(CMD.countReset())
      }
    }
  })
}

async function send(command) {
  if (!connected.value) {
    showToast('请先连接台灯')
    connectionOpen.value = true
    return
  }

  try {
    await ble.send(command)
  } catch (error) {
    showToast('发送失败')
    addLog(`发送失败：${formatError(error)}`)
  }
}

function shortDeviceId(id) {
  if (!id) return ''
  if (id.length <= 18) return id
  return `${id.slice(0, 8)}...${id.slice(-6)}`
}

function formatTime(date) {
  const h = String(date.getHours()).padStart(2, '0')
  const m = String(date.getMinutes()).padStart(2, '0')
  const s = String(date.getSeconds()).padStart(2, '0')
  return `${h}:${m}:${s}`
}

function formatError(error) {
  if (!error) return 'unknown'
  if (typeof error === 'string') return error
  return error.errMsg || error.message || JSON.stringify(error)
}

function showToast(title) {
  uni.showToast({ title, icon: 'none' })
}

function showLoading(title) {
  uni.showLoading({ title, mask: true })
}

function hideLoading() {
  uni.hideLoading()
}
</script>

<style scoped>
.page {
  min-height: 100vh;
  padding: 54rpx 30rpx 72rpx;
  box-sizing: border-box;
  color: #f8fafc;
  background:
    radial-gradient(circle at 50% -8%, rgba(255, 219, 128, 0.32), transparent 32%),
    radial-gradient(circle at 10% 20%, rgba(99, 102, 241, 0.16), transparent 28%),
    radial-gradient(circle at 95% 26%, rgba(125, 211, 252, 0.13), transparent 30%),
    linear-gradient(180deg, #10131a 0%, #080a0f 100%);
}

.topbar { display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 24rpx; }
.brand { display: block; font-size: 25rpx; color: rgba(248,250,252,.58); letter-spacing: 1rpx; }
.headline { display: block; margin-top: 8rpx; font-size: 42rpx; line-height: 1.15; font-weight: 800; letter-spacing: 0; }

.connect-dot { display: flex; align-items: center; gap: 10rpx; padding: 13rpx 20rpx; border-radius: 999rpx; font-size: 24rpx; font-weight: 700; background: rgba(255,255,255,.08); border: 1rpx solid rgba(255,255,255,.12); }
.connect-dot .dot { width: 14rpx; height: 14rpx; border-radius: 50%; }
.connect-dot.online .dot { background: #86efac; box-shadow: 0 0 18rpx rgba(134,239,172,.95); }
.connect-dot.offline .dot { background: #94a3b8; }

.lamp-stage { position: relative; height: 520rpx; display: flex; flex-direction: column; align-items: center; justify-content: center; }
.glow-ring { position: absolute; width: 420rpx; height: 420rpx; border-radius: 50%; filter: blur(8rpx); opacity: .78; }
.glow-ring.active { background: radial-gradient(circle, rgba(255,222,139,.58) 0%, rgba(255,222,139,.2) 38%, transparent 68%); animation: softPulse 3.2s ease-in-out infinite; }
.glow-ring.sleep { background: radial-gradient(circle, rgba(148,163,184,.18) 0%, rgba(148,163,184,.08) 40%, transparent 70%); }
.lamp-orb { position: relative; width: 218rpx; height: 218rpx; border-radius: 50%; display: flex; align-items: center; justify-content: center; border: 1rpx solid rgba(255,255,255,.2); box-shadow: inset 0 0 44rpx rgba(255,255,255,.05); }
.orb-active { background: linear-gradient(145deg, #fff4c2, #e8b94f); box-shadow: 0 0 52rpx rgba(255,217,118,.78), 0 0 160rpx rgba(255,217,118,.28), inset -20rpx -20rpx 48rpx rgba(166,107,0,.18); animation: floatOrb 4s ease-in-out infinite; }
.orb-sleep { background: linear-gradient(145deg, rgba(255,255,255,.12), rgba(255,255,255,.02)); box-shadow: inset -18rpx -18rpx 40rpx rgba(0,0,0,.28); }
.orb-inner { width: 74rpx; height: 74rpx; border-radius: 50%; background: rgba(255,255,255,.18); filter: blur(2rpx); }
.mode-title { margin-top: 34rpx; font-size: 44rpx; font-weight: 800; letter-spacing: 0; }
.mode-subtitle { margin-top: 12rpx; color: rgba(248,250,252,.54); font-size: 26rpx; }

.glass-card, .fold-card, .mini-card { background: rgba(255,255,255,.075); border: 1rpx solid rgba(255,255,255,.12); box-shadow: 0 24rpx 88rpx rgba(0,0,0,.24); backdrop-filter: blur(24rpx); }
.main-control { border-radius: 38rpx; padding: 26rpx; margin-bottom: 20rpx; }
.segmented { display: flex; padding: 8rpx; border-radius: 999rpx; background: rgba(0,0,0,.24); margin-bottom: 28rpx; }
.seg-item { flex: 1; height: 70rpx; line-height: 70rpx; text-align: center; border-radius: 999rpx; color: rgba(248,250,252,.56); font-weight: 800; font-size: 27rpx; transition: all .22s ease; }
.seg-item.selected { color: #10131a; background: linear-gradient(135deg, #fff0b2, #f5c84b); box-shadow: 0 10rpx 30rpx rgba(245,200,75,.25); }
.brightness-head { display: flex; justify-content: space-between; align-items: center; }
.small-label, .mini-label, .panel-desc, .fold-subtitle, .device-id { color: rgba(248,250,252,.48); font-size: 23rpx; }
.brightness-value { display: block; margin-top: 6rpx; font-size: 62rpx; line-height: 1; font-weight: 900; letter-spacing: 0; }
.tiny-btn { height: 58rpx; line-height: 58rpx; padding: 0 24rpx; border-radius: 999rpx; background: rgba(255,255,255,.09); color: rgba(248,250,252,.82); font-size: 24rpx; }
.slider { margin: 26rpx 0 18rpx; }
.quick-row, .count-actions, .connection-actions { display: flex; gap: 16rpx; align-items: center; }
.round-btn { width: 84rpx; height: 84rpx; line-height: 84rpx; border-radius: 50%; color: #f8fafc; font-size: 42rpx; background: rgba(255,255,255,.10); }
.mode-toggle-btn, .primary-action, .secondary-action, .soft-chip, .clear-btn { border-radius: 999rpx; font-weight: 800; }
.mode-toggle-btn { flex: 1; height: 84rpx; line-height: 84rpx; background: rgba(248,208,111,.16); color: #ffe49b; font-size: 27rpx; }
.compact-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 16rpx; margin-bottom: 20rpx; }
.mini-card { border-radius: 28rpx; padding: 22rpx 18rpx; }
.mini-value { display: block; margin-top: 10rpx; color: #f8fafc; font-size: 32rpx; font-weight: 900; }
.count-panel { border-radius: 34rpx; padding: 26rpx; margin-bottom: 20rpx; display: flex; justify-content: space-between; gap: 18rpx; align-items: center; }
.panel-title, .fold-title { display: block; font-size: 30rpx; color: #f8fafc; font-weight: 850; }
.panel-desc { display: block; margin-top: 9rpx; line-height: 1.45; }
.soft-chip { height: 64rpx; line-height: 64rpx; padding: 0 24rpx; background: rgba(255,255,255,.10); color: #f8fafc; font-size: 24rpx; }
.soft-chip.danger { color: #fecaca; background: rgba(239,68,68,.15); }
.fold-card { border-radius: 32rpx; margin-bottom: 18rpx; overflow: hidden; }
.fold-header { display: flex; justify-content: space-between; align-items: center; padding: 24rpx 26rpx; }
.chevron { font-size: 24rpx; color: rgba(248,250,252,.55); }
.fold-body { padding: 0 26rpx 26rpx; animation: slideDown .22s ease; }
.primary-action { flex: 1; height: 74rpx; line-height: 74rpx; color: #111827; background: linear-gradient(135deg, #fff0b2, #f5c84b); }
.secondary-action { width: 150rpx; height: 74rpx; line-height: 74rpx; color: #fecaca; background: rgba(239,68,68,.14); }
.empty { margin-top: 18rpx; padding: 22rpx; border-radius: 24rpx; color: rgba(248,250,252,.52); background: rgba(255,255,255,.06); }
.device { margin-top: 16rpx; padding: 20rpx; border-radius: 24rpx; display: flex; align-items: center; justify-content: space-between; background: rgba(255,255,255,.07); }
.device-name { display: block; font-size: 27rpx; font-weight: 800; color: #f8fafc; margin-bottom: 6rpx; }
.rssi { font-size: 23rpx; color: #f5d06f; }
.clear-btn { height: 58rpx; line-height: 58rpx; margin-bottom: 16rpx; color: rgba(248,250,252,.72); background: rgba(255,255,255,.08); font-size: 24rpx; }
.log { height: 330rpx; border-radius: 24rpx; background: rgba(0,0,0,.24); padding: 18rpx; box-sizing: border-box; }
.log-line { margin-bottom: 12rpx; color: rgba(248,250,252,.62); font-size: 22rpx; word-break: break-all; }
button::after { border: none; }

@keyframes softPulse { 0%,100% { transform: scale(.94); opacity: .58; } 50% { transform: scale(1.06); opacity: .92; } }
@keyframes floatOrb { 0%,100% { transform: translateY(0); } 50% { transform: translateY(-10rpx); } }
@keyframes slideDown { from { opacity: 0; transform: translateY(-8rpx); } to { opacity: 1; transform: translateY(0); } }
</style>
