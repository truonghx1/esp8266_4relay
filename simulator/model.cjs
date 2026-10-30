'use strict';
const fs = require('node:fs');
const path = require('node:path');
const { performance } = require('node:perf_hooks');

class ApiError extends Error {
  constructor(status, message) { super(message); this.status = status; }
}
const fail = (status, message) => { throw new ApiError(status, message); };
const integer = (v, min, max) => Number.isInteger(v) && v >= min && v <= max;
function validNames(names) {
  return Array.isArray(names) && names.length === 4 && names.every(name =>
    typeof name === 'string' && name.length > 0 && [...name].length <= 24 &&
    Buffer.byteLength(name, 'utf8') <= 96 && name.replace(/ /g, '').length > 0 &&
    !/[\u0000-\u001f\u007f-\u009f\u2028\u2029]/u.test(name) &&
    ![...name].some(char => char.codePointAt(0) >= 0xD800 && char.codePointAt(0) <= 0xDFFF));
}
function validateSchedule(s) {
  return typeof s.enabled === 'boolean' && integer(s.relays, 1, 15) &&
    integer(s.days, 1, 127) && integer(s.start, 0, 1439) &&
    integer(s.end, 0, 1439) && s.start !== s.end;
}
function activeMask(schedules, epoch, offset) {
  const date = new Date((epoch + offset * 60) * 1000);
  const day = (date.getUTCDay() + 6) % 7;
  const minute = date.getUTCHours() * 60 + date.getUTCMinutes();
  return schedules.reduce((mask, s) => {
    if (!s.enabled) return mask;
    const today = !!(s.days & (1 << day));
    const active = s.start < s.end ? today && minute >= s.start && minute < s.end :
      (today && minute >= s.start) || ((s.days & (1 << ((day + 6) % 7))) && minute < s.end);
    return active ? mask | s.relays : mask;
  }, 0);
}

class Simulator {
  constructor(file, now = () => performance.now()) {
    this.file = file;
    this.now = now;
    this.schedules = [];
    this.relayNames = [1, 2, 3, 4].map(id => 'Kênh ' + id);
    this.utcOffsetMinutes = 420;
    this.configHealthy = true;
    this.load();
    this.reboot();
  }
  load() {
    if (!fs.existsSync(this.file)) return;
    try {
      const data = JSON.parse(fs.readFileSync(this.file, 'utf8'));
      if (data.version !== 1 || !integer(data.utcOffsetMinutes, -720, 840) ||
          !Array.isArray(data.schedules) || data.schedules.length > 10 ||
          !data.schedules.every(s => integer(s.id, 0, 9) && validateSchedule(s)) ||
          new Set(data.schedules.map(s => s.id)).size !== data.schedules.length) throw Error('Invalid config');
      if (Object.hasOwn(data, 'relayNames') && !validNames(data.relayNames)) throw Error('Invalid names');
      this.schedules = data.schedules;
      if (data.relayNames) this.relayNames = data.relayNames.map(name => name.trim());
      this.utcOffsetMinutes = data.utcOffsetMinutes;
    } catch { this.configHealthy = false; }
  }
  save(schedules, offset, relayNames = this.relayNames) {
    try {
      fs.mkdirSync(path.dirname(this.file), { recursive: true });
      fs.writeFileSync(this.file + '.tmp', JSON.stringify({ version: 1, utcOffsetMinutes: offset, relayNames, schedules }, null, 2));
      fs.renameSync(this.file + '.tmp', this.file);
    } catch { fail(507, 'Không lưu được cấu hình mô phỏng. Thay đổi chưa được áp dụng.'); }
    this.configHealthy = true;
  }
  reboot() {
    this.timeValid = false;
    this.baseEpoch = 0;
    this.baseMillis = this.now();
    this.manual = 0;
    this.manualValues = 0;
    this.previous = 0;
    this.output = 0;
    // Pause automatic phone sync so the user can inspect the power-loss state.
    this.testClock = true;
  }
  epoch() { return this.timeValid ? this.baseEpoch + Math.floor((this.now() - this.baseMillis) / 1000) : 0; }
  evaluate(reset = false) {
    const scheduled = this.timeValid ? activeMask(this.schedules, this.epoch(), this.utcOffsetMinutes) : 0;
    this.manual &= ~(scheduled ^ this.previous);
    if (reset) this.manual = 0;
    this.previous = scheduled;
    this.output = (scheduled & ~this.manual) | (this.manualValues & this.manual);
  }
  sync(epoch, offset) {
    if (!integer(epoch, 1704067200, 4102444799) || !integer(offset, -720, 840))
      fail(400, 'Giờ phải nằm trong năm 2024–2099, múi giờ UTC−12 đến UTC+14.');
    if (offset !== this.utcOffsetMinutes) this.save(this.schedules, offset);
    const first = !this.timeValid;
    this.utcOffsetMinutes = offset;
    this.baseEpoch = epoch;
    this.baseMillis = this.now();
    this.timeValid = true;
    this.evaluate(first);
  }
  state() {
    this.evaluate();
    return {
      timeValid: this.timeValid, epoch: this.epoch(), utcOffsetMinutes: this.utcOffsetMinutes,
      storageReady: true, configHealthy: this.configHealthy, maxSchedules: 10,
      relays: [1, 2, 3, 4].map(id => ({ id, name: this.relayNames[id - 1], on: !!(this.output & (1 << (id - 1))), manual: !!(this.manual & (1 << (id - 1))) })),
      schedules: this.schedules.map(s => ({ ...s })),
      simulator: { testClock: this.testClock }
    };
  }
  command(route, body) {
    this.evaluate();
    if (route === '/api/time') {
      if (!this.testClock) this.sync(body.epoch, body.utcOffsetMinutes);
    } else if (route === '/api/names') {
      if (!validNames(body.names)) fail(400, 'Nhập đủ 4 tên, mỗi tên 1–24 ký tự, không chứa ký tự điều khiển.');
      const names = body.names.map(name => name.trim());
      if (names.some((name, i) => name !== this.relayNames[i])) {
        this.save(this.schedules, this.utcOffsetMinutes, names);
        this.relayNames = names;
      }
    } else if (route === '/api/relay' || route === '/api/auto') {
      if (!integer(body.relay, 1, 4) || (route === '/api/relay' && typeof body.on !== 'boolean'))
        fail(400, 'Relay phải từ 1 đến 4; on phải là true hoặc false.');
      const bit = 1 << (body.relay - 1);
      if (route === '/api/auto') this.manual &= ~bit;
      else {
        this.manual |= bit;
        this.manualValues = body.on ? this.manualValues | bit : this.manualValues & ~bit;
      }
    } else if (route === '/api/schedule' || route === '/api/schedule/delete') {
      const remove = route.endsWith('/delete');
      if (!integer(body.id, remove ? 0 : -1, 9)) fail(400, 'ID lịch không hợp lệ.');
      let id = body.id;
      if (id === -1) {
        id = Array.from({ length: 10 }, (_, i) => i).find(i => !this.schedules.some(s => s.id === i));
        if (id === undefined) fail(409, 'Đã đủ 10 lịch. Hãy sửa hoặc xóa lịch cũ.');
      } else if (!this.schedules.some(s => s.id === id)) fail(404, 'Lịch không còn tồn tại.');
      if (!remove && !validateSchedule(body)) fail(400, 'Chọn relay, thứ và hai giờ khác nhau.');
      const candidate = this.schedules.filter(s => s.id !== id);
      if (!remove) candidate.push({ id, enabled: body.enabled, relays: body.relays, days: body.days, start: body.start, end: body.end });
      candidate.sort((a, b) => a.id - b.id);
      this.save(candidate, this.utcOffsetMinutes);
      this.schedules = candidate;
      this.evaluate(true);
    } else if (route === '/sim/time' || route === '/sim/realtime') {
      this.sync(body.epoch, body.utcOffsetMinutes);
      this.testClock = route === '/sim/time';
    } else if (route === '/sim/advance') {
      if (!this.timeValid) fail(409, 'Hãy đặt giờ hoặc lấy giờ thật trước.');
      if (!integer(body.seconds, 1, 604800)) fail(400, 'Khoảng tua phải từ 1 giây đến 7 ngày.');
      const target = this.epoch() + body.seconds;
      if (target >= 4102444800) fail(400, 'Giờ thử nghiệm vượt năm 2099.');
      // Visit each minute boundary so manual overrides expire even if a jump
      // crosses a whole ON/OFF interval and ends with the original state.
      let next = Math.floor(this.epoch() / 60) * 60 + 60;
      while (next <= target) {
        this.baseEpoch = next; this.baseMillis = this.now(); this.evaluate(); next += 60;
      }
      this.baseEpoch = target; this.baseMillis = this.now(); this.testClock = true;
    } else if (route === '/sim/reboot') {
      this.reboot();
    } else fail(404, 'Không tìm thấy đường dẫn.');
    return this.state();
  }
}
module.exports = { Simulator, ApiError, activeMask };
