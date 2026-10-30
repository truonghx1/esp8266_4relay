// Install test dependency with:
// npm install --prefix .tools/ui-test --cache .tools/npm-cache jsdom --no-audit --no-fund
const assert = require('node:assert/strict');
const fs = require('node:fs');
const { JSDOM } = require('../.tools/ui-test/node_modules/jsdom');
const source = fs.readFileSync('firmware/esp8266_4relay/web_ui.h', 'utf8');
const html = source.match(/R"HTML\(([\s\S]*)\)HTML";/)[1];
const requests = [];
let nextError = null;
let state = {
  timeValid: false, epoch: 0, utcOffsetMinutes: 420,
  storageReady: true, configHealthy: true, maxSchedules: 10,
  wifi: { ssid: '', connected: false, status: 'Chưa cấu hình', ip: '', firebase: 'Chưa kết nối Wi-Fi' },
  relays: [1, 2, 3, 4].map(id => ({ id, on: false, manual: false })), schedules: []
};
const dom = new JSDOM(html, {
  url: 'http://192.168.4.1/', runScripts: 'dangerously', pretendToBeVisual: true,
  beforeParse(window) {
    window.TextEncoder = TextEncoder;
    window.confirm = () => true;
    window.HTMLDialogElement.prototype.showModal = function () { this.open = true; };
    window.HTMLDialogElement.prototype.close = function () { this.open = false; };
    window.fetch = async (path, options) => {
      const data = options.body ? JSON.parse(options.body) : undefined;
      requests.push({ path, data });
      if (nextError) { const error = nextError; nextError = null; return { ok: false, json: async () => ({ error }) }; }
      if (path === '/api/time') { state.timeValid = true; state.epoch = data.epoch; state.utcOffsetMinutes = data.utcOffsetMinutes; }
      if (path === '/api/relay') { const relay = state.relays[data.relay - 1]; relay.on = data.on; relay.manual = true; }
      if (path === '/api/auto') state.relays[data.relay - 1].manual = false;
      if (path === '/api/names') state.relays.forEach((relay, i) => { relay.name = data.names[i]; });
      if (path === '/api/wifi') state.wifi = { ...state.wifi, ssid: data.ssid, status: 'Đang kết nối' };
      if (path === '/api/schedule') {
        const id = data.id < 0 ? Array.from({ length: 10 }, (_, i) => i).find(i => !state.schedules.some(s => s.id === i)) : data.id;
        state.schedules = state.schedules.filter(s => s.id !== id).concat({ ...data, id });
      }
      if (path === '/api/schedule/delete') state.schedules = state.schedules.filter(s => s.id !== data.id);
      return { ok: true, json: async () => JSON.parse(JSON.stringify(state)) };
    };
  }
});
const window = dom.window;
const document = window.document;
const $ = id => document.getElementById(id);
const settle = () => new Promise(resolve => setTimeout(resolve, 20));
const submit = () => $('scheduleForm').dispatchEvent(new window.Event('submit', { bubbles: true, cancelable: true }));

(async () => {
  await settle();
  assert.equal(requests[0].path, '/api/state');
  assert.equal(requests[1].path, '/api/time');
  assert.ok(Number.isInteger(requests[1].data.epoch));
  assert.equal(document.querySelectorAll('.relay').length, 4);
  assert.equal($('connection').textContent, 'Đã kết nối');
  document.querySelector('#relays .switch').click();
  await settle();
  assert.equal(state.relays[0].on, true);
  assert.equal(document.querySelector('#relays .switch').getAttribute('aria-checked'), 'true');
  document.querySelector('#relays [aria-label="Tắt Kênh 1"]').click();
  await settle();
  assert.equal(state.relays[0].on, false);
  document.querySelector('#relays [aria-label="Bật Kênh 1"]').click();
  await settle();
  assert.equal(state.relays[0].on, true);

  $('add').click();
  assert.equal($('editor').open, true);
  document.querySelectorAll('#relayChoices input')[1].checked = true;
  document.querySelectorAll('#dayChoices input').forEach((input, i) => { input.checked = i === 6; });
  $('start').value = '22:00'; $('end').value = '02:00';
  $('start').dispatchEvent(new window.Event('input'));
  assert.match($('overnight').textContent, /Qua nửa đêm/);
  submit(); await settle();
  assert.equal(state.schedules.length, 1);
  assert.equal(state.schedules[0].relays, 3);
  assert.equal(state.schedules[0].days, 64);
  assert.equal(state.schedules[0].start, 1320);
  assert.equal(state.schedules[0].end, 120);
  assert.equal($('editor').open, false);
  assert.match($('schedules').textContent, /\(\+1 ngày\)/);

  document.querySelector('#schedules .schedule-edit').click();
  $('end').value = '22:00';
  const beforeInvalid = requests.length;
  submit(); await settle();
  assert.equal(requests.length, beforeInvalid);
  assert.equal($('formError').hidden, false);
  $('end').value = '03:00';
  nextError = 'Không lưu được flash.';
  submit(); await settle();
  assert.equal($('editor').open, true);
  assert.equal($('formError').textContent, 'Không lưu được flash.');
  assert.equal(state.schedules[0].end, 120);
  submit(); await settle();
  assert.equal(state.schedules[0].end, 180);
  assert.equal($('editor').open, false);

  document.querySelector('#schedules .switch').click(); await settle();
  assert.equal(state.schedules[0].enabled, false);
  document.querySelector('#schedules .schedule-more').click();
  assert.equal(document.querySelector('#schedules .actions').hidden, false);
  await window.refresh();
  assert.equal(document.querySelector('#schedules .actions').hidden, false, 'polling must keep the action menu open');
  document.querySelector('#schedules [data-action="delete"]').click(); await settle();
  assert.equal(state.schedules.length, 0);
  state.schedules = Array.from({ length: 10 }, (_, id) => ({ id, enabled: true, relays: 1, days: 127, start: 480, end: 540 }));
  await window.refresh();
  assert.equal($('add').disabled, true);
  assert.equal($('count').textContent, '10/10');
  assert.equal(document.querySelectorAll('.schedule').length, 10, 'home includes every saved schedule');
  $('schedules').scrollTop = 200;
  const tenthSchedule = document.querySelectorAll('.schedule')[9];
  await window.refresh();
  assert.equal(document.querySelectorAll('.schedule')[9], tenthSchedule, 'polling must not replace an unchanged scrolling list');
  assert.equal($('schedules').scrollTop, 200);
  tenthSchedule.querySelector('.schedule-edit').click();
  assert.equal($('editorTitle').textContent, 'Sửa lịch 10');
  $('cancel').click();
  $('viewAll').click();
  assert.equal(document.querySelectorAll('.schedule').length, 10);
  $('settingsShortcut').click();
  $('wifiSsid').value = 'My home'; $('wifiPassword').value = '12345678';
  const submitWifi = () => $('wifiForm').dispatchEvent(new window.Event('submit', { bubbles: true, cancelable: true }));
  nextError = 'Không lưu được Wi-Fi';
  submitWifi(); await settle();
  assert.equal($('wifiPassword').value, '12345678', 'failed save keeps credentials for retry');
  submitWifi(); await settle();
  assert.equal(requests.at(-1).path, '/api/wifi');
  assert.deepEqual(requests.at(-1).data, { ssid: 'My home', password: '12345678' });
  assert.equal($('wifiPassword').value, '');
  assert.match($('wifiStatus').textContent, /Đang kết nối/);
  $('wifiPassword').value = 'short'; const beforeBadWifi = requests.length;
  submitWifi(); await settle(); assert.equal(requests.length, beforeBadWifi);
  $('viewAll').click();
  assert.equal($('relaySection').hidden, true);
  assert.equal(document.querySelector('[data-page="schedules"]').getAttribute('aria-current'), 'page');
  document.querySelector('[data-page="device"]').click();
  assert.equal($('deviceSection').hidden, false);
  assert.equal($('deviceAddress').textContent, '192.168.4.1');
  $('settingsShortcut').click();
  assert.equal($('settingsSection').hidden, false);
  assert.equal($('deviceSection').hidden, true);
  const names = ['Đèn phòng khách', 'Quạt trần', 'Ổ cắm', '<b>Đèn</b>'];
  names.forEach((name, i) => {
    $('name' + (i + 1)).value = name;
    $('name' + (i + 1)).dispatchEvent(new window.Event('input', { bubbles: true }));
  });
  await window.refresh();
  assert.equal($('name1').value, names[0], 'polling must not overwrite unsaved edits');
  nextError = 'Không lưu được tên thiết bị.';
  const submitNames = () => $('namesForm').dispatchEvent(new window.Event('submit', { bubbles: true, cancelable: true }));
  submitNames(); await settle();
  assert.equal($('namesError').hidden, false);
  assert.equal($('name1').value, names[0]);
  assert.equal(state.relays[0].name, undefined);
  submitNames(); await settle();
  assert.equal($('namesError').hidden, true);
  assert.deepEqual(state.relays.map(r => r.name), names);
  assert.equal(document.querySelector('#relays h3').textContent, names[0]);
  assert.equal(document.querySelectorAll('#relays h3')[3].textContent, '<b>Đèn</b>');
  assert.equal(document.querySelector('#relays h3 b'), null);
  assert.ok($('schedules').textContent.includes(names[0]));
  document.querySelector('#schedules .schedule-edit').click();
  assert.ok($('relayChoices').textContent.includes(names[1]));
  $('cancel').click();
  $('name1').value = 'a'.repeat(25);
  $('name1').dispatchEvent(new window.Event('input'));
  const beforeBadName = requests.length;
  submitNames(); await settle();
  assert.equal(requests.length, beforeBadName);
  assert.equal($('namesError').hidden, false);
  document.querySelector('[data-page="home"]').click();
  assert.equal($('relaySection').hidden, false);
  assert.equal(document.querySelectorAll('.schedule').length, 10);
  console.log('PASS: relay switch + on/off, schedule CRUD + menus, clock, validation, capacity, all navigation tabs');
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(() => dom.window.close());
