/*
 * NightLight BLE UART Protocol v1
 *
 * APP -> lamp:   @COMMAND\r\n
 * lamp -> APP:   #TYPE,K=V,K=V\r\n
 */

export const CMD = {
  ping: () => '@PING\r\n',
  get: () => '@GET\r\n',
  status: () => '@STATUS?\r\n',

  modeAuto: () => '@MODE=AUTO\r\n',
  modeManual: () => '@MODE=MANUAL\r\n',
  modeToggle: () => '@MODE=TOGGLE\r\n',

  brightUp: () => '@BRIGHT+\r\n',
  brightDown: () => '@BRIGHT-\r\n',
  brightSet: (value) => `@BRIGHT=${clampInt(value, 0, 100)}\r\n`,

  countGet: () => '@COUNT?\r\n',
  countReset: () => '@COUNT_RESET\r\n',

  ledOn: () => '@LED=ON\r\n',
  ledOff: () => '@LED=OFF\r\n'
}

export function clampInt(value, min, max) {
  const n = Number.parseInt(value, 10)
  if (Number.isNaN(n)) return min
  return Math.max(min, Math.min(max, n))
}

export function strToArrayBuffer(str) {
  const buffer = new ArrayBuffer(str.length)
  const view = new Uint8Array(buffer)
  for (let i = 0; i < str.length; i += 1) {
    view[i] = str.charCodeAt(i) & 0xff
  }
  return buffer
}

export function arrayBufferToString(buffer) {
  const bytes = new Uint8Array(buffer)
  let result = ''
  for (let i = 0; i < bytes.length; i += 1) {
    result += String.fromCharCode(bytes[i])
  }
  return result
}

export function parseFrame(rawLine) {
  const line = String(rawLine || '').trim()
  if (!line) return null

  if (line === '#PONG') {
    return { type: 'PONG', fields: {}, raw: line }
  }

  if (!line.startsWith('#')) {
    return { type: 'RAW', fields: {}, raw: line }
  }

  const body = line.slice(1)
  const parts = body.split(',').filter(Boolean)
  const type = parts.shift() || 'UNKNOWN'
  const fields = {}

  parts.forEach((item) => {
    const index = item.indexOf('=')
    if (index > 0) {
      const key = item.slice(0, index).trim()
      const value = item.slice(index + 1).trim()
      fields[key] = value
    }
  })

  return {
    type,
    fields,
    raw: line
  }
}

export function applyFrameToStatus(currentStatus, frame) {
  if (!frame) return currentStatus

  const next = { ...currentStatus }

  if (frame.type === 'STATUS') {
    updateStatusFields(next, frame.fields)
  }

  if (frame.type === 'COUNT') {
    if (frame.fields.HUMAN_HIT !== undefined) {
      next.hit = Number.parseInt(frame.fields.HUMAN_HIT, 10) || 0
    }
  }

  if (frame.type === 'EVT') {
    updateStatusFields(next, frame.fields)

    if (frame.fields.MODE) {
      next.mode = frame.fields.MODE
    }

    if (frame.fields.LIGHT !== undefined) {
      next.light = frame.fields.LIGHT === '1'
    }

    if (frame.fields.HUMAN !== undefined) {
      next.human = frame.fields.HUMAN === '1'
    }

    if (frame.fields.HIT !== undefined) {
      next.hit = Number.parseInt(frame.fields.HIT, 10) || 0
    }

    if (frame.fields.BRIGHT !== undefined) {
      next.bright = Number.parseInt(frame.fields.BRIGHT, 10) || 0
    }

    if (frame.fields.TARGET !== undefined) {
      next.target = Number.parseInt(frame.fields.TARGET, 10) || 0
    }
  }

  return next
}

function updateStatusFields(status, fields) {
  if (fields.MODE !== undefined) status.mode = fields.MODE
  if (fields.LIGHT !== undefined) status.light = fields.LIGHT === '1'
  if (fields.BRIGHT !== undefined) status.bright = Number.parseInt(fields.BRIGHT, 10) || 0
  if (fields.TARGET !== undefined) status.target = Number.parseInt(fields.TARGET, 10) || 0
  if (fields.HUMAN !== undefined) status.human = fields.HUMAN === '1'
  if (fields.HIT !== undefined) status.hit = Number.parseInt(fields.HIT, 10) || 0
}
