import { strToArrayBuffer, arrayBufferToString } from './protocol.js'

function normalizeUuid(uuid) {
  return String(uuid || '').toLowerCase()
}

function isPreferredUartService(uuid) {
  const u = normalizeUuid(uuid)
  return u.includes('ffe0') || u.includes('fff0') || u.includes('ffe5') || u.includes('ae00')
}

function isPreferredUartChar(uuid) {
  const u = normalizeUuid(uuid)
  return u.includes('ffe1') || u.includes('fff1') || u.includes('ffe9') || u.includes('ae01')
}

export class BleUartClient {
  constructor(options = {}) {
    this.onDeviceFound = options.onDeviceFound || (() => {})
    this.onFrame = options.onFrame || (() => {})
    this.onRaw = options.onRaw || (() => {})
    this.onLog = options.onLog || (() => {})
    this.onState = options.onState || (() => {})

    this.deviceId = ''
    this.serviceId = ''
    this.writeCharId = ''
    this.notifyCharId = ''
    this.connected = false
    this.rxText = ''
    this.devices = new Map()

    uni.onBluetoothDeviceFound((result) => {
      const list = result.devices || []
      list.forEach((device) => {
        if (!device || !device.deviceId) return

        const name = device.name || device.localName || 'Unknown BLE'
        const item = {
          deviceId: device.deviceId,
          name,
          RSSI: device.RSSI || 0,
          raw: device
        }

        this.devices.set(device.deviceId, item)
        this.onDeviceFound(Array.from(this.devices.values()))
      })
    })

    uni.onBLEConnectionStateChange((res) => {
      this.connected = !!res.connected
      this.onState({
        connected: this.connected,
        deviceId: res.deviceId || this.deviceId
      })
      this.log(this.connected ? '设备已连接' : '设备已断开')
    })

    uni.onBLECharacteristicValueChange((res) => {
      const chunk = arrayBufferToString(res.value)
      this.onRaw(chunk)
      this.pushRxChunk(chunk)
    })
  }

  log(message) {
    this.onLog(message)
  }

  async open() {
    await callUni('openBluetoothAdapter')
    this.log('蓝牙适配器已打开')
  }

  async startScan() {
    this.devices.clear()

    try {
      await this.open()
    } catch (error) {
      this.log('打开蓝牙失败，请检查蓝牙和定位权限')
      throw error
    }

    await callUni('startBluetoothDevicesDiscovery', {
      allowDuplicatesKey: false
    })

    this.log('开始扫描 BLE 设备')
  }

  async stopScan() {
    try {
      await callUni('stopBluetoothDevicesDiscovery')
      this.log('已停止扫描')
    } catch (error) {
      this.log('停止扫描失败')
    }
  }

  async connect(deviceId) {
    if (!deviceId) throw new Error('deviceId is empty')

    await this.stopScan()

    await callUni('createBLEConnection', {
      deviceId,
      timeout: 10000
    })

    this.deviceId = deviceId
    this.connected = true
    this.log('连接成功，正在发现服务')

    await sleep(500)
    await this.discoverUart()
    await this.enableNotify()

    this.log('BLE UART 已就绪')
    this.onState({
      connected: true,
      deviceId: this.deviceId,
      serviceId: this.serviceId,
      writeCharId: this.writeCharId,
      notifyCharId: this.notifyCharId
    })
  }

  async disconnect() {
    if (!this.deviceId) return

    try {
      await callUni('closeBLEConnection', {
        deviceId: this.deviceId
      })
    } finally {
      this.connected = false
      this.log('已断开连接')
      this.onState({ connected: false })
    }
  }

  async discoverUart() {
    const serviceRes = await callUni('getBLEDeviceServices', {
      deviceId: this.deviceId
    })

    const services = serviceRes.services || []
    if (!services.length) throw new Error('没有发现 BLE 服务')

    const sorted = services.slice().sort((a, b) => {
      const ap = isPreferredUartService(a.uuid) ? 0 : 1
      const bp = isPreferredUartService(b.uuid) ? 0 : 1
      return ap - bp
    })

    for (const service of sorted) {
      const charRes = await callUni('getBLEDeviceCharacteristics', {
        deviceId: this.deviceId,
        serviceId: service.uuid
      })

      const chars = charRes.characteristics || []
      const notifyChar = pickCharacteristic(chars, ['notify', 'indicate'])
      const writeChar = pickCharacteristic(chars, ['write', 'writeNoResponse'])

      if (notifyChar && writeChar) {
        this.serviceId = service.uuid
        this.notifyCharId = notifyChar.uuid
        this.writeCharId = writeChar.uuid
        return
      }
    }

    throw new Error('没有找到同时支持 write 和 notify 的 BLE UART 特征值')
  }

  async enableNotify() {
    await callUni('notifyBLECharacteristicValueChange', {
      deviceId: this.deviceId,
      serviceId: this.serviceId,
      characteristicId: this.notifyCharId,
      state: true
    })
  }

  async send(command) {
    if (!this.connected) throw new Error('设备未连接')
    if (!this.serviceId || !this.writeCharId) throw new Error('BLE UART 未就绪')

    const text = command.endsWith('\r\n') ? command : `${command}\r\n`
    const chunks = splitByByteLength(text, 20)

    for (const chunk of chunks) {
      await callUni('writeBLECharacteristicValue', {
        deviceId: this.deviceId,
        serviceId: this.serviceId,
        characteristicId: this.writeCharId,
        value: strToArrayBuffer(chunk)
      })
      await sleep(35)
    }

    this.log(`发送：${text.replace(/\r\n/g, '\\r\\n')}`)
  }

  pushRxChunk(chunk) {
    this.rxText += chunk

    let index = this.rxText.indexOf('\r\n')
    while (index >= 0) {
      const line = this.rxText.slice(0, index)
      this.rxText = this.rxText.slice(index + 2)
      this.onFrame(line)
      index = this.rxText.indexOf('\r\n')
    }

    if (this.rxText.length > 256) {
      this.rxText = ''
      this.log('接收缓存过长，已清空')
    }
  }
}

function pickCharacteristic(chars, props) {
  const preferred = chars.find((item) => {
    if (!isPreferredUartChar(item.uuid)) return false
    return props.some((key) => item.properties && item.properties[key])
  })

  if (preferred) return preferred

  return chars.find((item) => {
    return props.some((key) => item.properties && item.properties[key])
  })
}

function splitByByteLength(text, maxLen) {
  const result = []
  for (let i = 0; i < text.length; i += maxLen) {
    result.push(text.slice(i, i + maxLen))
  }
  return result
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

function callUni(name, options = {}) {
  return new Promise((resolve, reject) => {
    uni[name]({
      ...options,
      success: resolve,
      fail: reject
    })
  })
}
